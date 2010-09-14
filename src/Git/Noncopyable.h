/**
* Noncopyable.h
* @Author   Tu Yongce <yongce (at) 126 (dot) com>
* @Created  2008-11-17
* @Modified 2008-11-17
* @Version  0.1
*/

#ifndef NONCOPYABLE_H_INCLUDED
#define NONCOPYABLE_H_INCLUDED

class Noncopyable
{
protected:
    // disallow instantiation
    Noncopyable() {}
    ~Noncopyable() {}

private:
    // forbid copy constructor & copy assignment operator
    Noncopyable(const Noncopyable&);
    Noncopyable& operator= (const Noncopyable&);
};

#endif // NONCOPYABLE_H_INCLUDED
