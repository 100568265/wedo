
#ifndef __SYSNULLPTR_H
#define __SYSNULLPTR_H

const class {
public:
template<class T>
    operator T* () const { return 0; }

template<class C, class T>
    operator T C::* () const { return 0; }

private:
    void operator& () const;
} w_nullptr;

#endif //__SYSNULLPTR_H