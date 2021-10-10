#include "lxd/str_split.h"

int main(int argc, char const *argv[])
{
	std::vector<std::string> v = absl::StrSplit("a,b,c", ",");
	return 0;
}