
template <>
struct DecodeMemPtr<2*sizeof(void (*)())> {
    typedef void* (*Func)(void*);

    struct MemPtr {
        union {
            Func func;
            long vtable_index;
        } u;

        long delta;
    };

    template <class O>
    static Delegate0<void*> Convert(void* (O::*mf)(void), O* o) {
        Delegate0<void*> ret;

        MemPtr* mp = (MemPtr*) &mf;
        ret.obj = (char*) o + mp->delta;

        if (mp->u.vtable_index & 1) {
            char* vtable = *(char**) ret.obj;
            long vt_off = mp->u.vtable_index - 1;
            ret.func = *(Func*) (vtable + vt_off);
        } else {
            ret.func = mp->u.func;
        }

        return ret;
    }
};
