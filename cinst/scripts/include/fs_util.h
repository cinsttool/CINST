#include <string>
#include <vector>

inline bool endsWith(const std::string& s,const std::string& sub){
    auto pos = s.rfind(sub);
    return (pos != -1) && (pos==(s.length()-sub.length()));
}
inline bool startsWith(const std::string& s,const std::string& sub){
    return s.find(sub)==(0);
}
void filesStartWith(const std::string& pathstr, std::vector<std::string>& files, const std::string& prefix);