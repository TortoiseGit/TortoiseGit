#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mythes.hxx"



MyThes::MyThes(const char* idxpath, const char * datpath)
{
    nw = 0;
    encoding = NULL;
    list = NULL;
    offst = NULL;

    if (thInitialize(idxpath, datpath) != 1) {
        fprintf(stderr,"Error - can't open %s or %s\n",idxpath, datpath);
        fflush(stderr);
        if (encoding) free((void*)encoding);
        if (list)  free((void*)list);
        if (offst) free((void*)offst);
        // did not initialize properly - throw exception?
    }
}


MyThes::~MyThes()
{
    if (thCleanup() != 1) {
        /* did not cleanup properly - throw exception? */
    }
    if (encoding) free((void*)encoding);
    encoding = NULL;
    list = NULL;
    offst = NULL;
}


int MyThes::thInitialize(const char* idxpath, const char* datpath)
{

    // open the index file
    FILE * pifile = fopen(idxpath,"r");
    if (!pifile) {
        return 0;
    } 

    char * wrd = (char *)calloc(1, MAX_WD_LEN);

    // parse in encoding and index size */    
    int len = readLine(pifile,wrd,MAX_WD_LEN);
    encoding = mystrdup(wrd);
    len = readLine(pifile,wrd,MAX_WD_LEN);
    int idxsz = atoi(wrd); 
    

    // now allocate list, offst for the given size
    list = (char**)calloc(idxsz,sizeof(char*));
    offst = (unsigned int*) calloc(idxsz,sizeof(unsigned int));

    if ( (!(list)) || (!(offst)) ) {
       fprintf(stderr,"Error - bad memory allocation\n");
       fflush(stderr);
       free((void *)wrd);
       fclose(pifile);
       return 0;
    }

    // now parse the remaining lines of the index
    len = readLine(pifile,wrd,MAX_WD_LEN);
    while (len > 0)
    { 
        int np = mystr_indexOfChar(wrd,'|');
        if (nw < idxsz) {
           if (np >= 0) {          
              *(wrd+np) = '\0';
              list[nw] = (char *)calloc(1,(np+1));
              memcpy((list[nw]),wrd,np);
              offst[nw] = atoi(wrd+np+1);
              nw++;
       }
        }
        len = readLine(pifile,wrd,MAX_WD_LEN);
    }

    free((void *)wrd);
    fclose(pifile);
    pifile=NULL;

    /* next open the data file */
    pdfile = fopen(datpath,"r");
    return pdfile ? 1 : 0;
}


int MyThes::thCleanup()
{
    /* first close the data file */
    if (pdfile) {
        fclose(pdfile);
        pdfile=NULL;
    }

    /* now free up all the allocated strings on the list */
    for (int i=0; i < nw; i++) 
    {
        if (list[i]) {
            free(list[i]);
            list[i] = 0;
        }
    }

    if (list)  free((void*)list);
    if (offst) free((void*)offst);

    nw = 0;
    return 1;
}



// lookup text in index and count of meanings and a list of meaning entries
// with each entry having a synonym count and pointer to an 
// array of char * (i.e the synonyms)
// 
// note: calling routine should call CleanUpAfterLookup with the original
// meaning point and count to properly deallocate memory

int MyThes::Lookup(const char * pText, int len, mentry** pme)
{ 

    *pme = NULL;

    // handle the case of missing file or file related errors
    if (! pdfile) return 0;

    long offset = 0;

    /* copy search word and make sure null terminated */
    char * wrd = (char *) calloc(1,(len+1));
    memcpy(wrd,pText,len);
  
    /* find it in the list */
    int idx = binsearch(wrd,list,nw);
    free(wrd);  
    if (idx < 0) return 0;

    // now seek to the offset
    offset = (long) offst[idx];
    int rc = fseek(pdfile,offset,SEEK_SET);
    if (rc) {
       return 0;
    }

    // grab the count of the number of meanings
    // and allocate a list of meaning entries
    char * buf = NULL;
    buf  = (char *) malloc( MAX_LN_LEN );
    if (!buf) return 0;
    readLine(pdfile, buf, (MAX_LN_LEN-1));
    int np = mystr_indexOfChar(buf,'|');
    if (np < 0) {
         free(buf);
         return 0;
    }          
    int nmeanings = atoi(buf+np+1);
    *pme = (mentry*) malloc( nmeanings * sizeof(mentry) );
    if (!(*pme)) {
        free(buf);
        return 0;
    }

    // now read in each meaning and parse it to get defn, count and synonym lists
    mentry* pm = *(pme);
    char dfn[MAX_WD_LEN];

    for (int j = 0; j < nmeanings; j++) {
        readLine(pdfile, buf, (MAX_LN_LEN-1));

        pm->count = 0;
        pm->psyns = NULL;
        pm->defn = NULL;

        // store away the part of speech for later use
        char * p = buf;
        char * pos = NULL;
        np = mystr_indexOfChar(p,'|');
        if (np >= 0) {
           *(buf+np) = '\0';
       pos = mystrdup(p);
           p = p + np + 1;
    } else {
          pos = mystrdup("");
        }
        
        // count the number of fields in the remaining line
        int nf = 1;
        char * d = p;
        np = mystr_indexOfChar(d,'|');        
        while ( np >= 0 ) {
      nf++;
          d = d + np + 1;
          np = mystr_indexOfChar(d,'|');          
    }
    pm->count = nf;
        pm->psyns = (char **) malloc(nf*sizeof(char*)); 
        
        // fill in the synonym list
        d = p;
        for (int j = 0; j < nf; j++) {
            np = mystr_indexOfChar(d,'|');
            if (np > 0) {
          *(d+np) = '\0';
              pm->psyns[j] = mystrdup(d);
              d = d + np + 1;
            } else {
              pm->psyns[j] = mystrdup(d);
        }            
        }

        // add pos to first synonym to create the definition
        int k = strlen(pos);
        int m = strlen(pm->psyns[0]);
        if ((k+m) < (MAX_WD_LEN - 1)) {
             strncpy(dfn,pos,k);
             *(dfn+k) = ' ';
             strncpy((dfn+k+1),(pm->psyns[0]),m+1);
             pm->defn = mystrdup(dfn);
    } else {
         pm->defn = mystrdup(pm->psyns[0]);
    }
        free(pos);
        pm++;

    }
    free(buf);
   
    return nmeanings;
} 



void MyThes::CleanUpAfterLookup(mentry ** pme, int nmeanings)
{ 

    if (nmeanings == 0) return;
    if ((*pme) == NULL) return;

    mentry * pm = *pme;
       
    for (int i = 0; i < nmeanings; i++) {
       int count = pm->count;
       for (int j = 0; j < count; j++) {
      if (pm->psyns[j]) free(pm->psyns[j]);
          pm->psyns[j] = NULL;
       }
       if (pm->psyns) free(pm->psyns);
       pm->psyns = NULL;
       if (pm->defn) free(pm->defn);
       pm->defn = NULL;
       pm->count = 0;
       pm++;
    }
    pm = *pme;
    free(pm);
    *pme = NULL;
    return;
}


// read a line of text from a text file stripping
// off the line terminator and replacing it with
// a null string terminator.
// returns:  -1 on error or the number of characters in
//             in the returning string

// A maximum of nc characters will be returned

int MyThes::readLine(FILE * pf, char * buf, int nc)
{
    
  if (fgets(buf,nc,pf)) {
    mychomp(buf);
    return strlen(buf);
  }
  return -1;
}


 
//  performs a binary search on null terminated character
//  strings
//
//  returns: -1 on not found
//           index of wrd in the list[]

int MyThes::binsearch(char * sw, char* clist[], int nlst)
{
    int lp, up, mp, j, indx;
    lp = 0;
    up = nlst-1;
    indx = -1;
    if (strcmp(sw, clist[lp]) < 0) return -1;
    if (strcmp(sw, clist[up]) > 0) return -1;
    while (indx < 0 ) {
        mp = (int)((lp+up) >> 1);
        j = strcmp(sw, clist[mp]);
        if ( j > 0) {
            lp = mp + 1;
        } else if (j < 0 ) {
            up = mp - 1;
        } else {
            indx = mp;
        }
        if (lp > up) return -1;      
    }
    return indx;
}

char * MyThes::get_th_encoding()
{
  if (encoding) return encoding;
  return NULL;
}


// string duplication routine
char * MyThes::mystrdup(const char * p)
{
  int sl = strlen(p) + 1;
  char * d = (char *)malloc(sl);
  if (d) {
    memcpy(d,p,sl);
    return d;
  }
  return NULL;
}

// remove cross-platform text line end characters
void MyThes::mychomp(char * s)
{
  int k = strlen(s);
  if ((k > 0) && ((*(s+k-1)=='\r') || (*(s+k-1)=='\n'))) *(s+k-1) = '\0';
  if ((k > 1) && (*(s+k-2) == '\r')) *(s+k-2) = '\0';
}


// return index of char in string
int MyThes::mystr_indexOfChar(const char * d, int c)
{
  char * p = strchr((char *)d,c);
  if (p) return (int)(p-d);
  return -1;
}

