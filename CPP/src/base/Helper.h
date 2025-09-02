#ifndef HELPER_H
#define HELPER_H

#include <functional>

#include "Define.h"

// 辅助类，借助RAII，延迟析构执行
class Delay
{
  public:
    using Invoke = std::function<void()>;
    Delay(Invoke &&invoke) : invoke_{std::move(invoke)} {}

    ~Delay()
    {
        if (invoke_)
        {
            invoke_();
        }
    }

    DISALLOW_COPY_AND_MOVE_OPERATOR(Delay);

  private:
    Invoke invoke_;
};

// 辅助宏
// 定义默认变量名(带行号)
#define DELAY_HELPER(invoke_func_) const auto UNIQUE_IDENTIFIER(delay_) = Delay(invoke_func_);

// DELAY_HELPER([]() {});

#endif // HELPER_H