
struct GenericBaseClass {};
struct GenericVirtualClass : virtual public GenericBaseClass {
    GenericVirtualClass* GetThis() {return this;}
};

template <>
struct DecodeMemPtr<sizeof(void (*)())> {
    template <class O>
    static Delegate0<void*> Convert(void* (O::*mf)(void), O* o) {
        typedef void* (*Func)(void*);
        Delegate0<void*> ret;
        ret.func = *(Func*) &mf;
        ret.obj = o;
        return ret;
    }
};

template <>
struct DecodeMemPtr<sizeof(void (*)()) + sizeof(int)> {
    struct MemPtr {
        void* (*func)(void*);
        int delta;
    };

    template <class O>
    static Delegate0<void*> Convert(void* (O::*mf)(void), O* o) {
        Delegate0<void*> ret;
        MemPtr* mp = (MemPtr*) &mf;
        ret.func = mp->func;
        ret.obj = (char*) o + mp->delta;
        return ret;
    }
};

template <>
struct DecodeMemPtr<sizeof(void (*)()) + 2*sizeof(int)> {
    struct MemPtr {
#ifdef DELEGATE_HAS_MFUNC
        void* (__fastcall *func)(void*,int);
#else
        void* (*func)(void*);
#endif
        int delta;
        int vbtable_index;
    };

    template <class O>
    static Delegate0<void*> Convert(void* (O::*mf)(void), O* o) {
      typedef GenericVirtualClass* (GenericVirtualClass::*ProbePtr)(void);

      Delegate0<void*> ret;
      MemPtr* mp = (MemPtr*) &mf;
      ret.func = mp->func;

      ProbePtr pp = &GenericVirtualClass::GetThis;
      mp->func = ((MemPtr*) &pp)->func;

      ret.obj = (o->*mf)();

      return ret;
    }
};
