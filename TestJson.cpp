#include "lxd/json.h"
#include <gtest/gtest.h>

TEST(Json, Read) {
    const char* json = R"({"code":200,"data":[{"create_time":"2022 - 01 - 18 11:34 : 30","content":"\u65b9\u6848\uff1a20210607105659\uff0c\u56de\u68c0\u9a73\u56de\uff01\u9a73\u56de\u539f\u56e0\uff1a\u9a73\u56de\u7406\u7531"},{"create_time":"2021 - 06 - 07 10:59 : 40","content":"\u65b9\u6848\uff1a20210607105659\u5f85\u56de\u68c0\uff01"}]})";
    ksJson* rootNode = ksJson_Create();
    std::vector<std::pair<std::string, std::string>> logs;
    const char** error = NULL;
    if (ksJson_ReadFromBuffer(rootNode, json, error)) {
        const ksJson* codeNode = ksJson_GetMemberByName(rootNode, "code");
        if (ksJson_GetInt32(codeNode, 0) == 200) {
            const ksJson* dataNode = ksJson_GetMemberByName(rootNode, "data");
            assert(dataNode);
            for (int i = 0; i < ksJson_GetMemberCount(dataNode); i++) {
                const ksJson* opt = ksJson_GetMemberByIndex(dataNode, i);
                const char* time = ksJson_GetString(ksJson_GetMemberByName(opt, "create_time"), "");
                const char* content = ksJson_GetString(ksJson_GetMemberByName(opt, "content"), "");
                logs.push_back({ time, content });
            }
        }
    }
    ksJson_Destroy(rootNode);
    EXPECT_EQ(logs.size(), 2);
    EXPECT_EQ(logs[0].first, "2022 - 01 - 18 11:34 : 30");
    EXPECT_EQ(logs[1].first, "2021 - 06 - 07 10:59 : 40");
}