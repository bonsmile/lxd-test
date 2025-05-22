#include "lxd/stl2ply.h"
#include "lxd/fileio.h"
#include <gtest/gtest.h>

TEST(Stl2Ply, FileExist) {
    EXPECT_EQ(lxd::FileExists(L"../../asset/Stanford_Bunny.stl"), true);
}

TEST(Stl2Ply, convert) {
    std::string stlBuffer = lxd::ReadFile(L"../../asset/Stanford_Bunny.stl");
    EXPECT_TRUE(stlBuffer.size() > 84);
    char* plyBuffer{};
    int plyBufferSize{};
    EXPECT_TRUE(stl2ply(stlBuffer.data(), stlBuffer.size(), &plyBuffer, &plyBufferSize));
    EXPECT_TRUE(lxd::WriteFile(L"Stanford_Bunny.ply", plyBuffer, plyBufferSize));
}