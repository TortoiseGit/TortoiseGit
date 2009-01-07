<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method = "html" encoding="iso-8859-1" />
	<xsl:template match="/Exception">	
	<html>
		<title></title>
		<body>
    
    <xsl:if test = "ExceptionRecord/@ExceptionAddress[1]">
      <H2>Exception</H2>
	  <TABLE BORDER="1">
	    <TR>
	       <TH>Exceptino Description</TH> <TH>Code</TH> <TH>Address</TH> <TH>Module</TH>
	       <xsl:if test="ExceptionRecord/@Filename[1]">
	         <TH>Filename</TH> <TH>Function</TH> <TH>Line</TH>
	       </xsl:if>
	    </TR>
	    <TR>
	       <TD>
	          <xsl:value-of select = "ExceptionRecord/@ExceptionDescription[1]" />
	        </TD>
	       <TD>
	          <xsl:value-of select = "ExceptionRecord/@ExceptionCode[1]" />
	        </TD>
	       <TD>
	          <xsl:value-of select = "ExceptionRecord/@ExceptionAddress[1]" />
	        </TD>
	       <TD>
	          <xsl:value-of select = "ExceptionRecord/@ModuleName[1]" />
	        </TD>
            <xsl:if test="ExceptionRecord/@Filename[1]">
            	<td>
            	<xsl:value-of select = "ExceptionRecord/@Filename[1]" />
            	</td>
            </xsl:if>
            
            <xsl:if test="ExceptionRecord/@FunctionName[1]">
	            <td>
	            <xsl:value-of select = "ExceptionRecord/@FunctionName[1]" />
	            <xsl:if test="ExceptionRecord/@FunctionDisplacement[1]">
	              + <xsl:value-of select = "ExceptionRecord/@FunctionDisplacement[1]" />
	            </xsl:if>
	            </td>
		            <xsl:if test="ExceptionRecord/@LineNumber[1]">
			            <td>
			            <xsl:value-of select = "ExceptionRecord/@LineNumber[1]" />
			            <xsl:if test="ExceptionRecord/@LineDisplacement[1]">
			              + <xsl:value-of select = "ExceptionRecord/@LineDisplacement[1]" />
			            </xsl:if>
			            </td>
		            </xsl:if>
            </xsl:if>
	    </TR>
	    </TABLE>
    </xsl:if>

	<xsl:if test="ApplicationDescription">
		<H2>Application Description</H2>
		<pre><xsl:value-of select = "ApplicationDescription" /> </pre>
	 </xsl:if>

	<xsl:if test="CallStack">
		<h2>Call Stack</h2>
		<table border="1">
		<tr> <th>#</th> <th> Return Address </th> <th>Module</th> <th>File</th> <th> Function </th> <th> Line </th> </tr>
		   <xsl:for-each select="CallStack/Frame">
		     <xsl:sort data-type="number" select="@FrameNumber[1]"/>
		         <tr>
		            <td>
		            <xsl:value-of select = "@FrameNumber[1]" />
		            </td>
		            <td>
		            <xsl:value-of select = "@ReturnAddress[1]" />
		            </td>
		            <xsl:if test="@ModuleName[1]">
		            	<td>
		            	<xsl:value-of select = "@ModuleName[1]" />
		            	</td>
		            </xsl:if>
		            <xsl:if test="not(@ModuleName[1])">
		            	<td>
		            	-
		            	</td>
		            </xsl:if>
		            <xsl:if test="@Filename[1]">
		            	<td>
		            	<xsl:value-of select = "@Filename[1]" />
		            	</td>
		            </xsl:if>
		            <xsl:if test="not(@Filename[1])">
		            	<td>
		            	-
		            	</td>
		            </xsl:if>
		            
		            <xsl:if test="@FunctionName[1]">
			            <td>
			            <xsl:value-of select = "@FunctionName[1]" />
			            <xsl:if test="@FunctionDisplacement[1]">
			              + <xsl:value-of select = "@FunctionDisplacement[1]" />
			            </xsl:if>
			            </td>
				            <xsl:if test="@LineNumber[1]">
					            <td>
					            <xsl:value-of select = "@LineNumber[1]" />
					            <xsl:if test="@LineDisplacement[1]">
					              + <xsl:value-of select = "@LineDisplacement[1]" />
					            </xsl:if>
					            </td>
				            </xsl:if>
		            </xsl:if>
				</tr>
		   </xsl:for-each>
		</table>
	</xsl:if>


	<xsl:if test="Modules/Module">
	<h2>Loaded Modules</h2>
	<table border="1">
	<tr> <th>Full Path</th> <th> Product Version </th> <th> File Version </th> <th>Timestamp</th> <th>Base Addr</th> <th>Size</th>  </tr>
	   <xsl:for-each select="Modules/Module">
	     <xsl:sort data-type="text" select="@FullPath[1]"/>
	         <tr>
	            <td>
	            <xsl:value-of select = "@FullPath[1]" />
	            </td>
	            <td>
	            <xsl:value-of select = "@ProductVersion[1]" />
	            </td>
	            <td>
	            <xsl:if test="@ProductVersion[1] != @FileVersion[1]">
	            <xsl:value-of select = "@FileVersion[1]" />
	            </xsl:if>
	            </td>
	            <td>
	            <xsl:value-of select = "@TimeStamp[1]" />
	            </td>
	            <td>
	            <xsl:value-of select = "@BaseAddress[1]" />
	            </td>
	            <td>
	            <xsl:value-of select = "@Size[1]" />
	            </td>
			</tr>
	   </xsl:for-each>
	</table>
	</xsl:if>

			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>

