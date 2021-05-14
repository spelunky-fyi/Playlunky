#pragma once

template<class FunT>
struct CallOnce
{
    CallOnce(FunT&& fun)
    {
        static bool already_run = false;
        if (!already_run)
        {
            fun();
            already_run = true;
        }
    }
};
