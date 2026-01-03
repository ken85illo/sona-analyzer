
template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
std::shared_ptr<T> MakeRef(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename U>
std::shared_ptr<T> StaticCast(const std::shared_ptr<U> &ptr) {
    return std::static_pointer_cast<T>(ptr);
}
