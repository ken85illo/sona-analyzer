#pragma once

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
std::shared_ptr<T> MakeRef(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}
