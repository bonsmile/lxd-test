// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lxd/strings/str_split.h"
#include "lxd/strings/str_replace.h"
#include "lxd/strings/str_cat.h"
#include "lxd/strings/match.h"
#include <deque>
#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
//#include "absl/base/dynamic_annotations.h"
//#include "absl/base/macros.h"
//#include "absl/container/btree_map.h"
//#include "absl/container/btree_set.h"
//#include "absl/container/flat_hash_map.h"
//#include "absl/container/node_hash_map.h"
//#include "absl/strings/numbers.h"

namespace {

    using ::testing::ElementsAre;
    using ::testing::Pair;
    using ::testing::UnorderedElementsAre;

    TEST(Split, TraitsTest) {
        static_assert(!absl::strings_internal::SplitterIsConvertibleTo<int>::value,
                      "");
        static_assert(
            !absl::strings_internal::SplitterIsConvertibleTo<std::string>::value, "");
        static_assert(absl::strings_internal::SplitterIsConvertibleTo<
                      std::vector<std::string>>::value,
                      "");
        static_assert(
            !absl::strings_internal::SplitterIsConvertibleTo<std::vector<int>>::value,
            "");
        static_assert(absl::strings_internal::SplitterIsConvertibleTo<
                      std::vector<std::string_view>>::value,
                      "");
        static_assert(absl::strings_internal::SplitterIsConvertibleTo<
                      std::map<std::string, std::string>>::value,
                      "");
        static_assert(absl::strings_internal::SplitterIsConvertibleTo<
                      std::map<std::string_view, std::string_view>>::value,
                      "");
        static_assert(!absl::strings_internal::SplitterIsConvertibleTo<
                      std::map<int, std::string>>::value,
                      "");
        static_assert(!absl::strings_internal::SplitterIsConvertibleTo<
                      std::map<std::string, int>>::value,
                      "");
    }

    // This tests the overall split API, which is made up of the absl::StrSplit()
    // function and the Delimiter objects in the absl:: namespace.
    // This TEST macro is outside of any namespace to require full specification of
    // namespaces just like callers will need to use.
    TEST(Split, APIExamples) {
        {
            // Passes string delimiter. Assumes the default of ByString.
            std::vector<std::string> v = absl::StrSplit("a,b,c", ",");  // NOLINT
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));

            // Equivalent to...
            using absl::ByString;
            v = absl::StrSplit("a,b,c", ByString(","));
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));

            // Equivalent to...
            EXPECT_THAT(absl::StrSplit("a,b,c", ByString(",")),
                        ElementsAre("a", "b", "c"));
        }

        {
            // Same as above, but using a single character as the delimiter.
            std::vector<std::string> v = absl::StrSplit("a,b,c", ',');
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));

            // Equivalent to...
            using absl::ByChar;
            v = absl::StrSplit("a,b,c", ByChar(','));
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Uses the Literal string "=>" as the delimiter.
            const std::vector<std::string> v = absl::StrSplit("a=>b=>c", "=>");
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // The substrings are returned as string_views, eliminating copying.
            std::vector<std::string_view> v = absl::StrSplit("a,b,c", ',');
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Leading and trailing empty substrings.
            std::vector<std::string> v = absl::StrSplit(",a,b,c,", ',');
            EXPECT_THAT(v, ElementsAre("", "a", "b", "c", ""));
        }

        {
            // Splits on a delimiter that is not found.
            std::vector<std::string> v = absl::StrSplit("abc", ',');
            EXPECT_THAT(v, ElementsAre("abc"));
        }

        {
            // Splits the input string into individual characters by using an empty
            // string as the delimiter.
            std::vector<std::string> v = absl::StrSplit("abc", "");
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Splits string data with embedded NUL characters, using NUL as the
            // delimiter. A simple delimiter of "\0" doesn't work because strlen() will
            // say that's the empty string when constructing the std::string_view
            // delimiter. Instead, a non-empty string containing NUL can be used as the
            // delimiter.
            std::string embedded_nulls("a\0b\0c", 5);
            std::string null_delim("\0", 1);
            std::vector<std::string> v = absl::StrSplit(embedded_nulls, null_delim);
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Stores first two split strings as the members in a std::pair.
            std::pair<std::string, std::string> p = absl::StrSplit("a,b,c", ',');
            EXPECT_EQ("a", p.first);
            EXPECT_EQ("b", p.second);
            // "c" is omitted because std::pair can hold only two elements.
        }

        {
            // Results stored in std::set<std::string>
            std::set<std::string> v = absl::StrSplit("a,b,c,a,b,c,a,b,c", ',');
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Uses a non-const char* delimiter.
            char a[] = ",";
            char* d = a + 0;
            std::vector<std::string> v = absl::StrSplit("a,b,c", d);
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Results split using either of , or ;
            using absl::ByAnyChar;
            std::vector<std::string> v = absl::StrSplit("a,b;c", ByAnyChar(",;"));
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Uses the SkipWhitespace predicate.
            using absl::SkipWhitespace;
            std::vector<std::string> v =
                absl::StrSplit(" a , ,,b,", ',', SkipWhitespace());
            EXPECT_THAT(v, ElementsAre(" a ", "b"));
        }

        {
            // Uses the ByLength delimiter.
            using absl::ByLength;
            std::vector<std::string> v = absl::StrSplit("abcdefg", ByLength(3));
            EXPECT_THAT(v, ElementsAre("abc", "def", "g"));
        }

        {
            // Different forms of initialization / conversion.
            std::vector<std::string> v1 = absl::StrSplit("a,b,c", ',');
            EXPECT_THAT(v1, ElementsAre("a", "b", "c"));
            std::vector<std::string> v2(absl::StrSplit("a,b,c", ','));
            EXPECT_THAT(v2, ElementsAre("a", "b", "c"));
            auto v3 = std::vector<std::string>(absl::StrSplit("a,b,c", ','));
            EXPECT_THAT(v3, ElementsAre("a", "b", "c"));
            v3 = absl::StrSplit("a,b,c", ',');
            EXPECT_THAT(v3, ElementsAre("a", "b", "c"));
        }

        {
            // Results stored in a std::map.
            std::map<std::string, std::string> m = absl::StrSplit("a,1,b,2,a,3", ',');
            EXPECT_EQ(2, m.size());
            EXPECT_EQ("3", m["a"]);
            EXPECT_EQ("2", m["b"]);
        }

        {
            // Results stored in a std::multimap.
            std::multimap<std::string, std::string> m =
                absl::StrSplit("a,1,b,2,a,3", ',');
            EXPECT_EQ(3, m.size());
            auto it = m.find("a");
            EXPECT_EQ("1", it->second);
            ++it;
            EXPECT_EQ("3", it->second);
            it = m.find("b");
            EXPECT_EQ("2", it->second);
        }

        {
            // Demonstrates use in a range-based for loop in C++11.
            std::string s = "x,x,x,x,x,x,x";
            for(std::string_view sp : absl::StrSplit(s, ',')) {
                EXPECT_EQ("x", sp);
            }
        }

        {
            // Demonstrates use with a Predicate in a range-based for loop.
            using absl::SkipWhitespace;
            std::string s = " ,x,,x,,x,x,x,,";
            for(std::string_view sp : absl::StrSplit(s, ',', SkipWhitespace())) {
                EXPECT_EQ("x", sp);
            }
        }

        {
            // Demonstrates a "smart" split to std::map using two separate calls to
            // absl::StrSplit. One call to split the records, and another call to split
            // the keys and values. This also uses the Limit delimiter so that the
            // std::string "a=b=c" will split to "a" -> "b=c".
            std::map<std::string, std::string> m;
            for(std::string_view sp : absl::StrSplit("a=b=c,d=e,f=,g", ',')) {
                m.insert(absl::StrSplit(sp, absl::MaxSplits('=', 1)));
            }
            EXPECT_EQ("b=c", m.find("a")->second);
            EXPECT_EQ("e", m.find("d")->second);
            EXPECT_EQ("", m.find("f")->second);
            EXPECT_EQ("", m.find("g")->second);
        }
    }

    //
    // Tests for SplitIterator
    //

    TEST(SplitIterator, Basics) {
        auto splitter = absl::StrSplit("a,b", ',');
        auto it = splitter.begin();
        auto end = splitter.end();

        EXPECT_NE(it, end);
        EXPECT_EQ("a", *it);  // tests dereference
        ++it;                 // tests preincrement
        EXPECT_NE(it, end);
        EXPECT_EQ("b",
                  std::string(it->data(), it->size()));  // tests dereference as ptr
        it++;                                            // tests postincrement
        EXPECT_EQ(it, end);
    }

    // Simple Predicate to skip a particular string.
    class Skip {
    public:
        explicit Skip(const std::string& s) : s_(s) {}
        bool operator()(std::string_view sp) { return sp != s_; }

    private:
        std::string s_;
    };

    TEST(SplitIterator, Predicate) {
        auto splitter = absl::StrSplit("a,b,c", ',', Skip("b"));
        auto it = splitter.begin();
        auto end = splitter.end();

        EXPECT_NE(it, end);
        EXPECT_EQ("a", *it);  // tests dereference
        ++it;                 // tests preincrement -- "b" should be skipped here.
        EXPECT_NE(it, end);
        EXPECT_EQ("c",
                  std::string(it->data(), it->size()));  // tests dereference as ptr
        it++;                                            // tests postincrement
        EXPECT_EQ(it, end);
    }

    TEST(SplitIterator, EdgeCases) {
        // Expected input and output, assuming a delimiter of ','
        struct {
            std::string in;
            std::vector<std::string> expect;
        } specs[] = {
            {"", {""}},
            {"foo", {"foo"}},
            {",", {"", ""}},
            {",foo", {"", "foo"}},
            {"foo,", {"foo", ""}},
            {",foo,", {"", "foo", ""}},
            {"foo,bar", {"foo", "bar"}},
        };

        for(const auto& spec : specs) {
            SCOPED_TRACE(spec.in);
            auto splitter = absl::StrSplit(spec.in, ',');
            auto it = splitter.begin();
            auto end = splitter.end();
            for(const auto& expected : spec.expect) {
                EXPECT_NE(it, end);
                EXPECT_EQ(expected, *it++);
            }
            EXPECT_EQ(it, end);
        }
    }

    TEST(Splitter, Const) {
        const auto splitter = absl::StrSplit("a,b,c", ',');
        EXPECT_THAT(splitter, ElementsAre("a", "b", "c"));
    }

    TEST(Split, EmptyAndNull) {
        // Attention: Splitting a null std::string_view is different than splitting
        // an empty std::string_view even though both string_views are considered
        // equal. This behavior is likely surprising and undesirable. However, to
        // maintain backward compatibility, there is a small "hack" in
        // str_split_internal.h that preserves this behavior. If that behavior is ever
        // changed/fixed, this test will need to be updated.
        EXPECT_THAT(absl::StrSplit(std::string_view(""), '-'), ElementsAre(""));
        EXPECT_THAT(absl::StrSplit(std::string_view(), '-'), ElementsAre());
    }

    TEST(SplitIterator, EqualityAsEndCondition) {
        auto splitter = absl::StrSplit("a,b,c", ',');
        auto it = splitter.begin();
        auto it2 = it;

        // Increments it2 twice to point to "c" in the input text.
        ++it2;
        ++it2;
        EXPECT_EQ("c", *it2);

        // This test uses a non-end SplitIterator as the terminating condition in a
        // for loop. This relies on SplitIterator equality for non-end SplitIterators
        // working correctly. At this point it2 points to "c", and we use that as the
        // "end" condition in this test.
        std::vector<std::string_view> v;
        for(; it != it2; ++it) {
            v.push_back(*it);
        }
        EXPECT_THAT(v, ElementsAre("a", "b"));
    }

    //
    // Tests for Splitter
    //

    TEST(Splitter, RangeIterators) {
        auto splitter = absl::StrSplit("a,b,c", ',');
        std::vector<std::string_view> output;
        for(const std::string_view& p : splitter) {
            output.push_back(p);
        }
        EXPECT_THAT(output, ElementsAre("a", "b", "c"));
    }

    // Some template functions for use in testing conversion operators
    template <typename ContainerType, typename Splitter>
    void TestConversionOperator(const Splitter& splitter) {
        ContainerType output = splitter;
        EXPECT_THAT(output, UnorderedElementsAre("a", "b", "c", "d"));
    }

    template <typename MapType, typename Splitter>
    void TestMapConversionOperator(const Splitter& splitter) {
        MapType m = splitter;
        EXPECT_THAT(m, UnorderedElementsAre(Pair("a", "b"), Pair("c", "d")));
    }

    template <typename FirstType, typename SecondType, typename Splitter>
    void TestPairConversionOperator(const Splitter& splitter) {
        std::pair<FirstType, SecondType> p = splitter;
        EXPECT_EQ(p, (std::pair<FirstType, SecondType>("a", "b")));
    }

    TEST(Splitter, ConversionOperator) {
        auto splitter = absl::StrSplit("a,b,c,d", ',');

        TestConversionOperator<std::vector<std::string_view>>(splitter);
        TestConversionOperator<std::vector<std::string>>(splitter);
        TestConversionOperator<std::list<std::string_view>>(splitter);
        TestConversionOperator<std::list<std::string>>(splitter);
        TestConversionOperator<std::deque<std::string_view>>(splitter);
        TestConversionOperator<std::deque<std::string>>(splitter);
        TestConversionOperator<std::set<std::string_view>>(splitter);
        TestConversionOperator<std::set<std::string>>(splitter);
        TestConversionOperator<std::multiset<std::string_view>>(splitter);
        TestConversionOperator<std::multiset<std::string>>(splitter);
        //TestConversionOperator<absl::btree_set<std::string_view>>(splitter);
        //TestConversionOperator<absl::btree_set<std::string>>(splitter);
        //TestConversionOperator<absl::btree_multiset<std::string_view>>(splitter);
        //TestConversionOperator<absl::btree_multiset<std::string>>(splitter);
        TestConversionOperator<std::unordered_set<std::string>>(splitter);

        // Tests conversion to map-like objects.

        TestMapConversionOperator<std::map<std::string_view, std::string_view>>(
            splitter);
        TestMapConversionOperator<std::map<std::string_view, std::string>>(splitter);
        TestMapConversionOperator<std::map<std::string, std::string_view>>(splitter);
        TestMapConversionOperator<std::map<std::string, std::string>>(splitter);
        TestMapConversionOperator<
            std::multimap<std::string_view, std::string_view>>(splitter);
        TestMapConversionOperator<std::multimap<std::string_view, std::string>>(
            splitter);
        TestMapConversionOperator<std::multimap<std::string, std::string_view>>(
            splitter);
        TestMapConversionOperator<std::multimap<std::string, std::string>>(splitter);
        //TestMapConversionOperator<
        //    absl::btree_map<std::string_view, std::string_view>>(splitter);
        //TestMapConversionOperator<absl::btree_map<std::string_view, std::string>>(
        //    splitter);
        //TestMapConversionOperator<absl::btree_map<std::string, std::string_view>>(
        //    splitter);
        //TestMapConversionOperator<absl::btree_map<std::string, std::string>>(
        //    splitter);
        //TestMapConversionOperator<
        //    absl::btree_multimap<std::string_view, std::string_view>>(splitter);
        //TestMapConversionOperator<
        //    absl::btree_multimap<std::string_view, std::string>>(splitter);
        //TestMapConversionOperator<
        //    absl::btree_multimap<std::string, std::string_view>>(splitter);
        //TestMapConversionOperator<absl::btree_multimap<std::string, std::string>>(
        //    splitter);
        TestMapConversionOperator<std::unordered_map<std::string, std::string>>(
            splitter);
        //TestMapConversionOperator<
        //    absl::node_hash_map<std::string_view, std::string_view>>(splitter);
        //TestMapConversionOperator<
        //    absl::node_hash_map<std::string_view, std::string>>(splitter);
        //TestMapConversionOperator<
        //    absl::node_hash_map<std::string, std::string_view>>(splitter);
        //TestMapConversionOperator<
        //    absl::flat_hash_map<std::string_view, std::string_view>>(splitter);
        //TestMapConversionOperator<
        //    absl::flat_hash_map<std::string_view, std::string>>(splitter);
        //TestMapConversionOperator<
        //    absl::flat_hash_map<std::string, std::string_view>>(splitter);

        // Tests conversion to std::pair

        TestPairConversionOperator<std::string_view, std::string_view>(splitter);
        TestPairConversionOperator<std::string_view, std::string>(splitter);
        TestPairConversionOperator<std::string, std::string_view>(splitter);
        TestPairConversionOperator<std::string, std::string>(splitter);
    }

    // A few additional tests for conversion to std::pair. This conversion is
    // different from others because a std::pair always has exactly two elements:
    // .first and .second. The split has to work even when the split has
    // less-than, equal-to, and more-than 2 strings.
    TEST(Splitter, ToPair) {
        {
            // Empty string
            std::pair<std::string, std::string> p = absl::StrSplit("", ',');
            EXPECT_EQ("", p.first);
            EXPECT_EQ("", p.second);
        }

        {
            // Only first
            std::pair<std::string, std::string> p = absl::StrSplit("a", ',');
            EXPECT_EQ("a", p.first);
            EXPECT_EQ("", p.second);
        }

        {
            // Only second
            std::pair<std::string, std::string> p = absl::StrSplit(",b", ',');
            EXPECT_EQ("", p.first);
            EXPECT_EQ("b", p.second);
        }

        {
            // First and second.
            std::pair<std::string, std::string> p = absl::StrSplit("a,b", ',');
            EXPECT_EQ("a", p.first);
            EXPECT_EQ("b", p.second);
        }

        {
            // First and second and then more stuff that will be ignored.
            std::pair<std::string, std::string> p = absl::StrSplit("a,b,c", ',');
            EXPECT_EQ("a", p.first);
            EXPECT_EQ("b", p.second);
            // "c" is omitted.
        }
    }

    TEST(Splitter, Predicates) {
        static const char kTestChars[] = ",a, ,b,";
        using absl::AllowEmpty;
        using absl::SkipEmpty;
        using absl::SkipWhitespace;

        {
            // No predicate. Does not skip empties.
            auto splitter = absl::StrSplit(kTestChars, ',');
            std::vector<std::string> v = splitter;
            EXPECT_THAT(v, ElementsAre("", "a", " ", "b", ""));
        }

        {
            // Allows empty strings. Same behavior as no predicate at all.
            auto splitter = absl::StrSplit(kTestChars, ',', AllowEmpty());
            std::vector<std::string> v_allowempty = splitter;
            EXPECT_THAT(v_allowempty, ElementsAre("", "a", " ", "b", ""));

            // Ensures AllowEmpty equals the behavior with no predicate.
            auto splitter_nopredicate = absl::StrSplit(kTestChars, ',');
            std::vector<std::string> v_nopredicate = splitter_nopredicate;
            EXPECT_EQ(v_allowempty, v_nopredicate);
        }

        {
            // Skips empty strings.
            auto splitter = absl::StrSplit(kTestChars, ',', SkipEmpty());
            std::vector<std::string> v = splitter;
            EXPECT_THAT(v, ElementsAre("a", " ", "b"));
        }

        {
            // Skips empty and all-whitespace strings.
            auto splitter = absl::StrSplit(kTestChars, ',', SkipWhitespace());
            std::vector<std::string> v = splitter;
            EXPECT_THAT(v, ElementsAre("a", "b"));
        }
    }

    //
    // Tests for StrSplit()
    //

    TEST(Split, Basics) {
        {
            // Doesn't really do anything useful because the return value is ignored,
            // but it should work.
            absl::StrSplit("a,b,c", ',');
        }

        {
            std::vector<std::string_view> v = absl::StrSplit("a,b,c", ',');
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            std::vector<std::string> v = absl::StrSplit("a,b,c", ',');
            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
        }

        {
            // Ensures that assignment works. This requires a little extra work with
            // C++11 because of overloads with initializer_list.
            std::vector<std::string> v;
            v = absl::StrSplit("a,b,c", ',');

            EXPECT_THAT(v, ElementsAre("a", "b", "c"));
            std::map<std::string, std::string> m;
            m = absl::StrSplit("a,b,c", ',');
            EXPECT_EQ(2, m.size());
            std::unordered_map<std::string, std::string> hm;
            hm = absl::StrSplit("a,b,c", ',');
            EXPECT_EQ(2, hm.size());
        }
    }

    std::string_view ReturnStringView() { return "Hello World"; }
    const char* ReturnConstCharP() { return "Hello World"; }
    char* ReturnCharP() { return const_cast<char*>("Hello World"); }

    TEST(Split, AcceptsCertainTemporaries) {
        std::vector<std::string> v;
        v = absl::StrSplit(ReturnStringView(), ' ');
        EXPECT_THAT(v, ElementsAre("Hello", "World"));
        v = absl::StrSplit(ReturnConstCharP(), ' ');
        EXPECT_THAT(v, ElementsAre("Hello", "World"));
        v = absl::StrSplit(ReturnCharP(), ' ');
        EXPECT_THAT(v, ElementsAre("Hello", "World"));
    }

    TEST(Split, Temporary) {
        // Use a std::string longer than the SSO length, so that when the temporary is
        // destroyed, if the splitter keeps a reference to the string's contents,
        // it'll reference freed memory instead of just dead on-stack memory.
        const char input[] = "a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u";
        EXPECT_LT(sizeof(std::string), sizeof(input) / sizeof(input[0]))
            << "Input should be larger than fits on the stack.";

        // This happens more often in C++11 as part of a range-based for loop.
        auto splitter = absl::StrSplit(std::string(input), ',');
        std::string expected = "a";
        for(std::string_view letter : splitter) {
            EXPECT_EQ(expected, letter);
            ++expected[0];
        }
        EXPECT_EQ("v", expected);

        // This happens more often in C++11 as part of a range-based for loop.
        auto std_splitter = absl::StrSplit(std::string(input), ',');
        expected = "a";
        for(std::string_view letter : std_splitter) {
            EXPECT_EQ(expected, letter);
            ++expected[0];
        }
        EXPECT_EQ("v", expected);
    }

    template <typename T>
    static std::unique_ptr<T> CopyToHeap(const T& value) {
        return std::unique_ptr<T>(new T(value));
    }

    TEST(Split, LvalueCaptureIsCopyable) {
        std::string input = "a,b";
        auto heap_splitter = CopyToHeap(absl::StrSplit(input, ','));
        auto stack_splitter = *heap_splitter;
        heap_splitter.reset();
        std::vector<std::string> result = stack_splitter;
        EXPECT_THAT(result, testing::ElementsAre("a", "b"));
    }

    TEST(Split, TemporaryCaptureIsCopyable) {
        auto heap_splitter = CopyToHeap(absl::StrSplit(std::string("a,b"), ','));
        auto stack_splitter = *heap_splitter;
        heap_splitter.reset();
        std::vector<std::string> result = stack_splitter;
        EXPECT_THAT(result, testing::ElementsAre("a", "b"));
    }

    TEST(Split, SplitterIsCopyableAndMoveable) {
        auto a = absl::StrSplit("foo", '-');

        // Ensures that the following expressions compile.
        auto b = a;             // Copy construct
        auto c = std::move(a);  // Move construct
        b = c;                  // Copy assign
        c = std::move(b);       // Move assign

        EXPECT_THAT(c, ElementsAre("foo"));
    }

    TEST(Split, StringDelimiter) {
        {
            std::vector<std::string_view> v = absl::StrSplit("a,b", ',');
            EXPECT_THAT(v, ElementsAre("a", "b"));
        }

        {
            std::vector<std::string_view> v = absl::StrSplit("a,b", std::string(","));
            EXPECT_THAT(v, ElementsAre("a", "b"));
        }

        {
            std::vector<std::string_view> v =
                absl::StrSplit("a,b", std::string_view(","));
            EXPECT_THAT(v, ElementsAre("a", "b"));
        }
    }

#if !defined(__cpp_char8_t)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++2a-compat"
#endif
    TEST(Split, UTF8) {
        // Tests splitting utf8 strings and utf8 delimiters.
        std::string utf8_string = u8"\u03BA\u1F79\u03C3\u03BC\u03B5";
        {
            // A utf8 input string with an ascii delimiter.
            std::string to_split = "a," + utf8_string;
            std::vector<std::string_view> v = absl::StrSplit(to_split, ',');
            EXPECT_THAT(v, ElementsAre("a", utf8_string));
        }

        {
            // A utf8 input string and a utf8 delimiter.
            std::string to_split = "a," + utf8_string + ",b";
            std::string unicode_delimiter = "," + utf8_string + ",";
            std::vector<std::string_view> v =
                absl::StrSplit(to_split, unicode_delimiter);
            EXPECT_THAT(v, ElementsAre("a", "b"));
        }

        {
            // A utf8 input string and ByAnyChar with ascii chars.
            std::vector<std::string_view> v =
                absl::StrSplit(u8"Foo h\u00E4llo th\u4E1Ere", absl::ByAnyChar(" \t"));
            EXPECT_THAT(v, ElementsAre("Foo", u8"h\u00E4llo", u8"th\u4E1Ere"));
        }
    }
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif  // !defined(__cpp_char8_t)

    TEST(Split, EmptyStringDelimiter) {
        {
            std::vector<std::string> v = absl::StrSplit("", "");
            EXPECT_THAT(v, ElementsAre(""));
        }

        {
            std::vector<std::string> v = absl::StrSplit("a", "");
            EXPECT_THAT(v, ElementsAre("a"));
        }

        {
            std::vector<std::string> v = absl::StrSplit("ab", "");
            EXPECT_THAT(v, ElementsAre("a", "b"));
        }

        {
            std::vector<std::string> v = absl::StrSplit("a b", "");
            EXPECT_THAT(v, ElementsAre("a", " ", "b"));
        }
    }

    TEST(Split, SubstrDelimiter) {
        std::vector<std::string_view> results;
        std::string_view delim("//");

        results = absl::StrSplit("", delim);
        EXPECT_THAT(results, ElementsAre(""));

        results = absl::StrSplit("//", delim);
        EXPECT_THAT(results, ElementsAre("", ""));

        results = absl::StrSplit("ab", delim);
        EXPECT_THAT(results, ElementsAre("ab"));

        results = absl::StrSplit("ab//", delim);
        EXPECT_THAT(results, ElementsAre("ab", ""));

        results = absl::StrSplit("ab/", delim);
        EXPECT_THAT(results, ElementsAre("ab/"));

        results = absl::StrSplit("a/b", delim);
        EXPECT_THAT(results, ElementsAre("a/b"));

        results = absl::StrSplit("a//b", delim);
        EXPECT_THAT(results, ElementsAre("a", "b"));

        results = absl::StrSplit("a///b", delim);
        EXPECT_THAT(results, ElementsAre("a", "/b"));

        results = absl::StrSplit("a////b", delim);
        EXPECT_THAT(results, ElementsAre("a", "", "b"));
    }

    TEST(Split, EmptyResults) {
        std::vector<std::string_view> results;

        results = absl::StrSplit("", '#');
        EXPECT_THAT(results, ElementsAre(""));

        results = absl::StrSplit("#", '#');
        EXPECT_THAT(results, ElementsAre("", ""));

        results = absl::StrSplit("#cd", '#');
        EXPECT_THAT(results, ElementsAre("", "cd"));

        results = absl::StrSplit("ab#cd#", '#');
        EXPECT_THAT(results, ElementsAre("ab", "cd", ""));

        results = absl::StrSplit("ab##cd", '#');
        EXPECT_THAT(results, ElementsAre("ab", "", "cd"));

        results = absl::StrSplit("ab##", '#');
        EXPECT_THAT(results, ElementsAre("ab", "", ""));

        results = absl::StrSplit("ab#ab#", '#');
        EXPECT_THAT(results, ElementsAre("ab", "ab", ""));

        results = absl::StrSplit("aaaa", 'a');
        EXPECT_THAT(results, ElementsAre("", "", "", "", ""));

        results = absl::StrSplit("", '#', absl::SkipEmpty());
        EXPECT_THAT(results, ElementsAre());
    }

    template <typename Delimiter>
    static bool IsFoundAtStartingPos(std::string_view text, Delimiter d,
                                     size_t starting_pos, int expected_pos) {
        std::string_view found = d.Find(text, starting_pos);
        return found.data() != text.data() + text.size() &&
            expected_pos == found.data() - text.data();
    }

    // Helper function for testing Delimiter objects. Returns true if the given
    // Delimiter is found in the given string at the given position. This function
    // tests two cases:
    //   1. The actual text given, staring at position 0
    //   2. The text given with leading padding that should be ignored
    template <typename Delimiter>
    static bool IsFoundAt(std::string_view text, Delimiter d, int expected_pos) {
        const std::string leading_text = ",x,y,z,";
        return IsFoundAtStartingPos(text, d, 0, expected_pos) &&
            IsFoundAtStartingPos(leading_text + std::string(text), d,
                                 leading_text.length(),
                                 expected_pos + leading_text.length());
    }

    //
    // Tests for ByString
    //

    // Tests using any delimiter that represents a single comma.
    template <typename Delimiter>
    void TestComma(Delimiter d) {
        EXPECT_TRUE(IsFoundAt(",", d, 0));
        EXPECT_TRUE(IsFoundAt("a,", d, 1));
        EXPECT_TRUE(IsFoundAt(",b", d, 0));
        EXPECT_TRUE(IsFoundAt("a,b", d, 1));
        EXPECT_TRUE(IsFoundAt("a,b,", d, 1));
        EXPECT_TRUE(IsFoundAt("a,b,c", d, 1));
        EXPECT_FALSE(IsFoundAt("", d, -1));
        EXPECT_FALSE(IsFoundAt(" ", d, -1));
        EXPECT_FALSE(IsFoundAt("a", d, -1));
        EXPECT_FALSE(IsFoundAt("a b c", d, -1));
        EXPECT_FALSE(IsFoundAt("a;b;c", d, -1));
        EXPECT_FALSE(IsFoundAt(";", d, -1));
    }

    TEST(Delimiter, ByString) {
        using absl::ByString;
        TestComma(ByString(","));

        // Works as named variable.
        ByString comma_string(",");
        TestComma(comma_string);

        // The first occurrence of empty string ("") in a string is at position 0.
        // There is a test below that demonstrates this for std::string_view::find().
        // If the ByString delimiter returned position 0 for this, there would
        // be an infinite loop in the SplitIterator code. To avoid this, empty string
        // is a special case in that it always returns the item at position 1.
        std::string_view abc("abc");
        EXPECT_EQ(0, abc.find(""));  // "" is found at position 0
        ByString empty("");
        EXPECT_FALSE(IsFoundAt("", empty, 0));
        EXPECT_FALSE(IsFoundAt("a", empty, 0));
        EXPECT_TRUE(IsFoundAt("ab", empty, 1));
        EXPECT_TRUE(IsFoundAt("abc", empty, 1));
    }

    TEST(Split, ByChar) {
        using absl::ByChar;
        TestComma(ByChar(','));

        // Works as named variable.
        ByChar comma_char(',');
        TestComma(comma_char);
    }

    //
    // Tests for ByAnyChar
    //

    TEST(Delimiter, ByAnyChar) {
        using absl::ByAnyChar;
        ByAnyChar one_delim(",");
        // Found
        EXPECT_TRUE(IsFoundAt(",", one_delim, 0));
        EXPECT_TRUE(IsFoundAt("a,", one_delim, 1));
        EXPECT_TRUE(IsFoundAt("a,b", one_delim, 1));
        EXPECT_TRUE(IsFoundAt(",b", one_delim, 0));
        // Not found
        EXPECT_FALSE(IsFoundAt("", one_delim, -1));
        EXPECT_FALSE(IsFoundAt(" ", one_delim, -1));
        EXPECT_FALSE(IsFoundAt("a", one_delim, -1));
        EXPECT_FALSE(IsFoundAt("a;b;c", one_delim, -1));
        EXPECT_FALSE(IsFoundAt(";", one_delim, -1));

        ByAnyChar two_delims(",;");
        // Found
        EXPECT_TRUE(IsFoundAt(",", two_delims, 0));
        EXPECT_TRUE(IsFoundAt(";", two_delims, 0));
        EXPECT_TRUE(IsFoundAt(",;", two_delims, 0));
        EXPECT_TRUE(IsFoundAt(";,", two_delims, 0));
        EXPECT_TRUE(IsFoundAt(",;b", two_delims, 0));
        EXPECT_TRUE(IsFoundAt(";,b", two_delims, 0));
        EXPECT_TRUE(IsFoundAt("a;,", two_delims, 1));
        EXPECT_TRUE(IsFoundAt("a,;", two_delims, 1));
        EXPECT_TRUE(IsFoundAt("a;,b", two_delims, 1));
        EXPECT_TRUE(IsFoundAt("a,;b", two_delims, 1));
        // Not found
        EXPECT_FALSE(IsFoundAt("", two_delims, -1));
        EXPECT_FALSE(IsFoundAt(" ", two_delims, -1));
        EXPECT_FALSE(IsFoundAt("a", two_delims, -1));
        EXPECT_FALSE(IsFoundAt("a=b=c", two_delims, -1));
        EXPECT_FALSE(IsFoundAt("=", two_delims, -1));

        // ByAnyChar behaves just like ByString when given a delimiter of empty
        // string. That is, it always returns a zero-length std::string_view
        // referring to the item at position 1, not position 0.
        ByAnyChar empty("");
        EXPECT_FALSE(IsFoundAt("", empty, 0));
        EXPECT_FALSE(IsFoundAt("a", empty, 0));
        EXPECT_TRUE(IsFoundAt("ab", empty, 1));
        EXPECT_TRUE(IsFoundAt("abc", empty, 1));
    }

    //
    // Tests for ByLength
    //

    TEST(Delimiter, ByLength) {
        using absl::ByLength;

        ByLength four_char_delim(4);

        // Found
        EXPECT_TRUE(IsFoundAt("abcde", four_char_delim, 4));
        EXPECT_TRUE(IsFoundAt("abcdefghijklmnopqrstuvwxyz", four_char_delim, 4));
        EXPECT_TRUE(IsFoundAt("a b,c\nd", four_char_delim, 4));
        // Not found
        EXPECT_FALSE(IsFoundAt("", four_char_delim, 0));
        EXPECT_FALSE(IsFoundAt("a", four_char_delim, 0));
        EXPECT_FALSE(IsFoundAt("ab", four_char_delim, 0));
        EXPECT_FALSE(IsFoundAt("abc", four_char_delim, 0));
        EXPECT_FALSE(IsFoundAt("abcd", four_char_delim, 0));
    }

    TEST(Split, WorksWithLargeStrings) {
        if(sizeof(size_t) > 4) {
            std::string s((uint32_t{1} << 31) + 1, 'x');  // 2G + 1 byte
            s.back() = '-';
            std::vector<std::string_view> v = absl::StrSplit(s, '-');
            EXPECT_EQ(2, v.size());
            // The first element will contain 2G of 'x's.
            // testing::StartsWith is too slow with a 2G string.
            EXPECT_EQ('x', v[0][0]);
            EXPECT_EQ('x', v[0][1]);
            EXPECT_EQ('x', v[0][3]);
            EXPECT_EQ("", v[1]);
        }
    }

    TEST(SplitInternalTest, TypeTraits) {
        EXPECT_FALSE(absl::strings_internal::HasMappedType<int>::value);
        EXPECT_TRUE(
            (absl::strings_internal::HasMappedType<std::map<int, int>>::value));
        EXPECT_FALSE(absl::strings_internal::HasValueType<int>::value);
        EXPECT_TRUE(
            (absl::strings_internal::HasValueType<std::map<int, int>>::value));
        EXPECT_FALSE(absl::strings_internal::HasConstIterator<int>::value);
        EXPECT_TRUE(
            (absl::strings_internal::HasConstIterator<std::map<int, int>>::value));
        EXPECT_FALSE(absl::strings_internal::IsInitializerList<int>::value);
        EXPECT_TRUE((absl::strings_internal::IsInitializerList<
                     std::initializer_list<int>>::value));
    }


    ///
    ///
    /// 
    TEST(StrReplaceAll, OneReplacement) {
        std::string s;

        // Empty string.
        s = absl::StrReplaceAll(s, {{"", ""}});
        EXPECT_EQ(s, "");
        s = absl::StrReplaceAll(s, {{"x", ""}});
        EXPECT_EQ(s, "");
        s = absl::StrReplaceAll(s, {{"", "y"}});
        EXPECT_EQ(s, "");
        s = absl::StrReplaceAll(s, {{"x", "y"}});
        EXPECT_EQ(s, "");

        // Empty substring.
        s = absl::StrReplaceAll("abc", {{"", ""}});
        EXPECT_EQ(s, "abc");
        s = absl::StrReplaceAll("abc", {{"", "y"}});
        EXPECT_EQ(s, "abc");
        s = absl::StrReplaceAll("abc", {{"x", ""}});
        EXPECT_EQ(s, "abc");

        // Substring not found.
        s = absl::StrReplaceAll("abc", {{"xyz", "123"}});
        EXPECT_EQ(s, "abc");

        // Replace entire string.
        s = absl::StrReplaceAll("abc", {{"abc", "xyz"}});
        EXPECT_EQ(s, "xyz");

        // Replace once at the start.
        s = absl::StrReplaceAll("abc", {{"a", "x"}});
        EXPECT_EQ(s, "xbc");

        // Replace once in the middle.
        s = absl::StrReplaceAll("abc", {{"b", "x"}});
        EXPECT_EQ(s, "axc");

        // Replace once at the end.
        s = absl::StrReplaceAll("abc", {{"c", "x"}});
        EXPECT_EQ(s, "abx");

        // Replace multiple times with varying lengths of original/replacement.
        s = absl::StrReplaceAll("ababa", {{"a", "xxx"}});
        EXPECT_EQ(s, "xxxbxxxbxxx");

        s = absl::StrReplaceAll("ababa", {{"b", "xxx"}});
        EXPECT_EQ(s, "axxxaxxxa");

        s = absl::StrReplaceAll("aaabaaabaaa", {{"aaa", "x"}});
        EXPECT_EQ(s, "xbxbx");

        s = absl::StrReplaceAll("abbbabbba", {{"bbb", "x"}});
        EXPECT_EQ(s, "axaxa");

        // Overlapping matches are replaced greedily.
        s = absl::StrReplaceAll("aaa", {{"aa", "x"}});
        EXPECT_EQ(s, "xa");

        // The replacements are not recursive.
        s = absl::StrReplaceAll("aaa", {{"aa", "a"}});
        EXPECT_EQ(s, "aa");
    }

    TEST(StrReplaceAll, ManyReplacements) {
        std::string s;

        // Empty string.
        s = absl::StrReplaceAll("", {{"", ""}, {"x", ""}, {"", "y"}, {"x", "y"}});
        EXPECT_EQ(s, "");

        // Empty substring.
        s = absl::StrReplaceAll("abc", {{"", ""}, {"", "y"}, {"x", ""}});
        EXPECT_EQ(s, "abc");

        // Replace entire string, one char at a time
        s = absl::StrReplaceAll("abc", {{"a", "x"}, {"b", "y"}, {"c", "z"}});
        EXPECT_EQ(s, "xyz");
        s = absl::StrReplaceAll("zxy", {{"z", "x"}, {"x", "y"}, {"y", "z"}});
        EXPECT_EQ(s, "xyz");

        // Replace once at the start (longer matches take precedence)
        s = absl::StrReplaceAll("abc", {{"a", "x"}, {"ab", "xy"}, {"abc", "xyz"}});
        EXPECT_EQ(s, "xyz");

        // Replace once in the middle.
        s = absl::StrReplaceAll(
            "Abc!", {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc", "yz"}, {"c", "z"}});
        EXPECT_EQ(s, "Ayz!");

        // Replace once at the end.
        s = absl::StrReplaceAll(
            "Abc!",
            {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc!", "yz?"}, {"c!", "z;"}});
        EXPECT_EQ(s, "Ayz?");

        // Replace multiple times with varying lengths of original/replacement.
        s = absl::StrReplaceAll("ababa", {{"a", "xxx"}, {"b", "XXXX"}});
        EXPECT_EQ(s, "xxxXXXXxxxXXXXxxx");

        // Overlapping matches are replaced greedily.
        s = absl::StrReplaceAll("aaa", {{"aa", "x"}, {"a", "X"}});
        EXPECT_EQ(s, "xX");
        s = absl::StrReplaceAll("aaa", {{"a", "X"}, {"aa", "x"}});
        EXPECT_EQ(s, "xX");

        // Two well-known sentences
        s = absl::StrReplaceAll("the quick brown fox jumped over the lazy dogs",
                                {
                                    {"brown", "box"},
                                    {"dogs", "jugs"},
                                    {"fox", "with"},
                                    {"jumped", "five"},
                                    {"over", "dozen"},
                                    {"quick", "my"},
                                    {"the", "pack"},
                                    {"the lazy", "liquor"},
                                });
        EXPECT_EQ(s, "pack my box with five dozen liquor jugs");
    }

    TEST(StrReplaceAll, ManyReplacementsInMap) {
        std::map<const char*, const char*> replacements;
        replacements["$who"] = "Bob";
        replacements["$count"] = "5";
        replacements["#Noun"] = "Apples";
        std::string s = absl::StrReplaceAll("$who bought $count #Noun. Thanks $who!",
                                            replacements);
        EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
    }

    TEST(StrReplaceAll, ReplacementsInPlace) {
        std::string s = std::string("$who bought $count #Noun. Thanks $who!");
        int count;
        count = absl::StrReplaceAll({{"$count", absl::StrCat(5)},
                                    {"$who", "Bob"},
                                    {"#Noun", "Apples"}}, &s);
        EXPECT_EQ(count, 4);
        EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
    }

    TEST(StrReplaceAll, ReplacementsInPlaceInMap) {
        std::string s = std::string("$who bought $count #Noun. Thanks $who!");
        std::map<std::string_view, std::string_view> replacements;
        replacements["$who"] = "Bob";
        replacements["$count"] = "5";
        replacements["#Noun"] = "Apples";
        int count;
        count = absl::StrReplaceAll(replacements, &s);
        EXPECT_EQ(count, 4);
        EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
    }

    struct Cont {
        Cont() {}
        explicit Cont(std::string_view src) : data(src) {}

        std::string_view data;
    };

    template <int index>
    std::string_view get(const Cont& c) {
        auto splitter = absl::StrSplit(c.data, ':');
        auto it = splitter.begin();
        for(int i = 0; i < index; ++i) ++it;

        return *it;
    }

    TEST(StrReplaceAll, VariableNumber) {
        std::string s;
        {
            std::vector<std::pair<std::string, std::string>> replacements;

            s = "abc";
            EXPECT_EQ(0, absl::StrReplaceAll(replacements, &s));
            EXPECT_EQ("abc", s);

            s = "abc";
            replacements.push_back({"a", "A"});
            EXPECT_EQ(1, absl::StrReplaceAll(replacements, &s));
            EXPECT_EQ("Abc", s);

            s = "abc";
            replacements.push_back({"b", "B"});
            EXPECT_EQ(2, absl::StrReplaceAll(replacements, &s));
            EXPECT_EQ("ABc", s);

            s = "abc";
            replacements.push_back({"d", "D"});
            EXPECT_EQ(2, absl::StrReplaceAll(replacements, &s));
            EXPECT_EQ("ABc", s);

            EXPECT_EQ("ABcABc", absl::StrReplaceAll("abcabc", replacements));
        }

        {
            std::map<const char*, const char*> replacements;
            replacements["aa"] = "x";
            replacements["a"] = "X";
            s = "aaa";
            EXPECT_EQ(2, absl::StrReplaceAll(replacements, &s));
            EXPECT_EQ("xX", s);

            EXPECT_EQ("xxX", absl::StrReplaceAll("aaaaa", replacements));
        }

        {
            std::list<std::pair<std::string_view, std::string_view>> replacements = {
                {"a", "x"}, {"b", "y"}, {"c", "z"}};

            std::string s = absl::StrReplaceAll("abc", replacements);
            EXPECT_EQ(s, "xyz");
        }

        {
            using X = std::tuple<std::string_view, std::string, int>;
            std::vector<X> replacements(3);
            replacements[0] = X{"a", "x", 1};
            replacements[1] = X{"b", "y", 0};
            replacements[2] = X{"c", "z", -1};

            std::string s = absl::StrReplaceAll("abc", replacements);
            EXPECT_EQ(s, "xyz");
        }

        {
            std::vector<Cont> replacements(3);
            replacements[0] = Cont{"a:x"};
            replacements[1] = Cont{"b:y"};
            replacements[2] = Cont{"c:z"};

            std::string s = absl::StrReplaceAll("abc", replacements);
            EXPECT_EQ(s, "xyz");
        }
    }

    // Same as above, but using the in-place variant of absl::StrReplaceAll,
    // that returns the # of replacements performed.
    TEST(StrReplaceAll, Inplace) {
        std::string s;
        int reps;

        // Empty string.
        s = "";
        reps = absl::StrReplaceAll({{"", ""}, {"x", ""}, {"", "y"}, {"x", "y"}}, &s);
        EXPECT_EQ(reps, 0);
        EXPECT_EQ(s, "");

        // Empty substring.
        s = "abc";
        reps = absl::StrReplaceAll({{"", ""}, {"", "y"}, {"x", ""}}, &s);
        EXPECT_EQ(reps, 0);
        EXPECT_EQ(s, "abc");

        // Replace entire string, one char at a time
        s = "abc";
        reps = absl::StrReplaceAll({{"a", "x"}, {"b", "y"}, {"c", "z"}}, &s);
        EXPECT_EQ(reps, 3);
        EXPECT_EQ(s, "xyz");
        s = "zxy";
        reps = absl::StrReplaceAll({{"z", "x"}, {"x", "y"}, {"y", "z"}}, &s);
        EXPECT_EQ(reps, 3);
        EXPECT_EQ(s, "xyz");

        // Replace once at the start (longer matches take precedence)
        s = "abc";
        reps = absl::StrReplaceAll({{"a", "x"}, {"ab", "xy"}, {"abc", "xyz"}}, &s);
        EXPECT_EQ(reps, 1);
        EXPECT_EQ(s, "xyz");

        // Replace once in the middle.
        s = "Abc!";
        reps = absl::StrReplaceAll(
            {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc", "yz"}, {"c", "z"}}, &s);
        EXPECT_EQ(reps, 1);
        EXPECT_EQ(s, "Ayz!");

        // Replace once at the end.
        s = "Abc!";
        reps = absl::StrReplaceAll(
            {{"a", "x"}, {"ab", "xy"}, {"b", "y"}, {"bc!", "yz?"}, {"c!", "z;"}}, &s);
        EXPECT_EQ(reps, 1);
        EXPECT_EQ(s, "Ayz?");

        // Replace multiple times with varying lengths of original/replacement.
        s = "ababa";
        reps = absl::StrReplaceAll({{"a", "xxx"}, {"b", "XXXX"}}, &s);
        EXPECT_EQ(reps, 5);
        EXPECT_EQ(s, "xxxXXXXxxxXXXXxxx");

        // Overlapping matches are replaced greedily.
        s = "aaa";
        reps = absl::StrReplaceAll({{"aa", "x"}, {"a", "X"}}, &s);
        EXPECT_EQ(reps, 2);
        EXPECT_EQ(s, "xX");
        s = "aaa";
        reps = absl::StrReplaceAll({{"a", "X"}, {"aa", "x"}}, &s);
        EXPECT_EQ(reps, 2);
        EXPECT_EQ(s, "xX");

        // Two well-known sentences
        s = "the quick brown fox jumped over the lazy dogs";
        reps = absl::StrReplaceAll(
            {
                {"brown", "box"},
                {"dogs", "jugs"},
                {"fox", "with"},
                {"jumped", "five"},
                {"over", "dozen"},
                {"quick", "my"},
                {"the", "pack"},
                {"the lazy", "liquor"},
            },
            &s);
        EXPECT_EQ(reps, 8);
        EXPECT_EQ(s, "pack my box with five dozen liquor jugs");
    }
    ///
    ///
    ///
    // Test absl::StrCat of ints and longs of various sizes and signdedness.
    TEST(StrCat, Ints) {
        const short s = -1;  // NOLINT(runtime/int)
        const uint16_t us = 2;
        const int i = -3;
        const unsigned int ui = 4;
        const long l = -5;                 // NOLINT(runtime/int)
        const unsigned long ul = 6;        // NOLINT(runtime/int)
        const long long ll = -7;           // NOLINT(runtime/int)
        const unsigned long long ull = 8;  // NOLINT(runtime/int)
        const ptrdiff_t ptrdiff = -9;
        const size_t size = 10;
        const intptr_t intptr = -12;
        const uintptr_t uintptr = 13;
        std::string answer;
        answer = absl::StrCat(s, us);
        EXPECT_EQ(answer, "-12");
        answer = absl::StrCat(i, ui);
        EXPECT_EQ(answer, "-34");
        answer = absl::StrCat(l, ul);
        EXPECT_EQ(answer, "-56");
        answer = absl::StrCat(ll, ull);
        EXPECT_EQ(answer, "-78");
        answer = absl::StrCat(ptrdiff, size);
        EXPECT_EQ(answer, "-910");
        answer = absl::StrCat(ptrdiff, intptr);
        EXPECT_EQ(answer, "-9-12");
        answer = absl::StrCat(uintptr, 0);
        EXPECT_EQ(answer, "130");
    }

    TEST(StrCat, Enums) {
        enum SmallNumbers { One = 1, Ten = 10 } e = Ten;
        EXPECT_EQ("10", absl::StrCat(e));
        EXPECT_EQ("-5", absl::StrCat(SmallNumbers(-5)));

        enum class Option { Boxers = 1, Briefs = -1 };

        EXPECT_EQ("-1", absl::StrCat(Option::Briefs));

        enum class Airplane : uint64_t {
            Airbus = 1,
            Boeing = 1000,
            Canary = 10000000000  // too big for "int"
        };

        EXPECT_EQ("10000000000", absl::StrCat(Airplane::Canary));

        enum class TwoGig : int32_t {
            TwoToTheZero = 1,
            TwoToTheSixteenth = 1 << 16,
            TwoToTheThirtyFirst = INT32_MIN
        };
        EXPECT_EQ("65536", absl::StrCat(TwoGig::TwoToTheSixteenth));
        EXPECT_EQ("-2147483648", absl::StrCat(TwoGig::TwoToTheThirtyFirst));
        EXPECT_EQ("-1", absl::StrCat(static_cast<TwoGig>(-1)));

        enum class FourGig : uint32_t {
            TwoToTheZero = 1,
            TwoToTheSixteenth = 1 << 16,
            TwoToTheThirtyFirst = 1U << 31  // too big for "int"
        };
        EXPECT_EQ("65536", absl::StrCat(FourGig::TwoToTheSixteenth));
        EXPECT_EQ("2147483648", absl::StrCat(FourGig::TwoToTheThirtyFirst));
        EXPECT_EQ("4294967295", absl::StrCat(static_cast<FourGig>(-1)));

        EXPECT_EQ("10000000000", absl::StrCat(Airplane::Canary));
    }

    TEST(StrCat, Basics) {
        std::string result;

        std::string strs[] = {"Hello", "Cruel", "World"};

        std::string stdstrs[] = {
          "std::Hello",
          "std::Cruel",
          "std::World"
        };

        std::string_view pieces[] = {"Hello", "Cruel", "World"};

        const char* c_strs[] = {
          "Hello",
          "Cruel",
          "World"
        };

        int32_t i32s[] = {'H', 'C', 'W'};
        uint64_t ui64s[] = {12345678910LL, 10987654321LL};

        EXPECT_EQ(absl::StrCat(), "");

        result = absl::StrCat(false, true, 2, 3);
        EXPECT_EQ(result, "0123");

        result = absl::StrCat(-1);
        EXPECT_EQ(result, "-1");

        result = absl::StrCat(absl::SixDigits(0.5));
        EXPECT_EQ(result, "0.5");

        result = absl::StrCat(strs[1], pieces[2]);
        EXPECT_EQ(result, "CruelWorld");

        result = absl::StrCat(stdstrs[1], " ", stdstrs[2]);
        EXPECT_EQ(result, "std::Cruel std::World");

        result = absl::StrCat(strs[0], ", ", pieces[2]);
        EXPECT_EQ(result, "Hello, World");

        result = absl::StrCat(strs[0], ", ", strs[1], " ", strs[2], "!");
        EXPECT_EQ(result, "Hello, Cruel World!");

        result = absl::StrCat(pieces[0], ", ", pieces[1], " ", pieces[2]);
        EXPECT_EQ(result, "Hello, Cruel World");

        result = absl::StrCat(c_strs[0], ", ", c_strs[1], " ", c_strs[2]);
        EXPECT_EQ(result, "Hello, Cruel World");

        result = absl::StrCat("ASCII ", i32s[0], ", ", i32s[1], " ", i32s[2], "!");
        EXPECT_EQ(result, "ASCII 72, 67 87!");

        result = absl::StrCat(ui64s[0], ", ", ui64s[1], "!");
        EXPECT_EQ(result, "12345678910, 10987654321!");

        std::string one =
            "1";  // Actually, it's the size of this string that we want; a
                  // 64-bit build distinguishes between size_t and uint64_t,
                  // even though they're both unsigned 64-bit values.
        result = absl::StrCat("And a ", one.size(), " and a ",
                              &result[2] - &result[0], " and a ", one, " 2 3 4", "!");
        EXPECT_EQ(result, "And a 1 and a 2 and a 1 2 3 4!");

        // result = absl::StrCat("Single chars won't compile", '!');
        // result = absl::StrCat("Neither will nullptrs", nullptr);
        result =
            absl::StrCat("To output a char by ASCII/numeric value, use +: ", '!' + 0);
        EXPECT_EQ(result, "To output a char by ASCII/numeric value, use +: 33");

        float f = 100000.5;
        result = absl::StrCat("A hundred K and a half is ", absl::SixDigits(f));
        EXPECT_EQ(result, "A hundred K and a half is 100000");

        f = 100001.5;
        result =
            absl::StrCat("A hundred K and one and a half is ", absl::SixDigits(f));
        EXPECT_EQ(result, "A hundred K and one and a half is 100002");

        double d = 100000.5;
        d *= d;
        result =
            absl::StrCat("A hundred K and a half squared is ", absl::SixDigits(d));
        EXPECT_EQ(result, "A hundred K and a half squared is 1.00001e+10");

        result = absl::StrCat(1, 2, 333, 4444, 55555, 666666, 7777777, 88888888,
                              999999999);
        EXPECT_EQ(result, "12333444455555666666777777788888888999999999");
    }

    TEST(StrCat, CornerCases) {
        std::string result;

        result = absl::StrCat("");  // NOLINT
        EXPECT_EQ(result, "");
        result = absl::StrCat("", "");
        EXPECT_EQ(result, "");
        result = absl::StrCat("", "", "");
        EXPECT_EQ(result, "");
        result = absl::StrCat("", "", "", "");
        EXPECT_EQ(result, "");
        result = absl::StrCat("", "", "", "", "");
        EXPECT_EQ(result, "");
    }

    // A minimal allocator that uses malloc().
    template <typename T>
    struct Mallocator {
        typedef T value_type;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;

        size_type max_size() const {
            return size_t(std::numeric_limits<size_type>::max()) / sizeof(value_type);
        }
        template <typename U>
        struct rebind {
            typedef Mallocator<U> other;
        };
        Mallocator() = default;
        template <class U>
        Mallocator(const Mallocator<U>&) {}  // NOLINT(runtime/explicit)

        T* allocate(size_t n) { return static_cast<T*>(std::malloc(n * sizeof(T))); }
        void deallocate(T* p, size_t) { std::free(p); }
    };
    template <typename T, typename U>
    bool operator==(const Mallocator<T>&, const Mallocator<U>&) {
        return true;
    }
    template <typename T, typename U>
    bool operator!=(const Mallocator<T>&, const Mallocator<U>&) {
        return false;
    }

    TEST(StrCat, CustomAllocator) {
        using mstring =
            std::basic_string<char, std::char_traits<char>, Mallocator<char>>;
        const mstring str1("PARACHUTE OFF A BLIMP INTO MOSCONE!!");

        const mstring str2("Read this book about coffee tables");

        std::string result = absl::StrCat(str1, str2);
        EXPECT_EQ(result,
                  "PARACHUTE OFF A BLIMP INTO MOSCONE!!"
                  "Read this book about coffee tables");
    }

    TEST(StrCat, MaxArgs) {
        std::string result;
        // Test 10 up to 26 arguments, the old maximum
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a");
        EXPECT_EQ(result, "123456789a");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b");
        EXPECT_EQ(result, "123456789ab");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c");
        EXPECT_EQ(result, "123456789abc");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d");
        EXPECT_EQ(result, "123456789abcd");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e");
        EXPECT_EQ(result, "123456789abcde");
        result =
            absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f");
        EXPECT_EQ(result, "123456789abcdef");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g");
        EXPECT_EQ(result, "123456789abcdefg");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h");
        EXPECT_EQ(result, "123456789abcdefgh");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i");
        EXPECT_EQ(result, "123456789abcdefghi");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j");
        EXPECT_EQ(result, "123456789abcdefghij");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k");
        EXPECT_EQ(result, "123456789abcdefghijk");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k", "l");
        EXPECT_EQ(result, "123456789abcdefghijkl");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k", "l", "m");
        EXPECT_EQ(result, "123456789abcdefghijklm");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k", "l", "m", "n");
        EXPECT_EQ(result, "123456789abcdefghijklmn");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k", "l", "m", "n", "o");
        EXPECT_EQ(result, "123456789abcdefghijklmno");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k", "l", "m", "n", "o", "p");
        EXPECT_EQ(result, "123456789abcdefghijklmnop");
        result = absl::StrCat(1, 2, 3, 4, 5, 6, 7, 8, 9, "a", "b", "c", "d", "e", "f",
                              "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q");
        EXPECT_EQ(result, "123456789abcdefghijklmnopq");
        // No limit thanks to C++11's variadic templates
        result = absl::StrCat(
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, "a", "b", "c", "d", "e", "f", "g", "h",
            "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
            "x", "y", "z", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
            "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z");
        EXPECT_EQ(result,
                  "12345678910abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    }

    TEST(StrAppend, Basics) {
        std::string result = "existing text";

        std::string strs[] = {"Hello", "Cruel", "World"};

        std::string stdstrs[] = {
          "std::Hello",
          "std::Cruel",
          "std::World"
        };

        std::string_view pieces[] = {"Hello", "Cruel", "World"};

        const char* c_strs[] = {
          "Hello",
          "Cruel",
          "World"
        };

        int32_t i32s[] = {'H', 'C', 'W'};
        uint64_t ui64s[] = {12345678910LL, 10987654321LL};

        std::string::size_type old_size = result.size();
        absl::StrAppend(&result);
        EXPECT_EQ(result.size(), old_size);

        old_size = result.size();
        absl::StrAppend(&result, strs[0]);
        EXPECT_EQ(result.substr(old_size), "Hello");

        old_size = result.size();
        absl::StrAppend(&result, strs[1], pieces[2]);
        EXPECT_EQ(result.substr(old_size), "CruelWorld");

        old_size = result.size();
        absl::StrAppend(&result, stdstrs[0], ", ", pieces[2]);
        EXPECT_EQ(result.substr(old_size), "std::Hello, World");

        old_size = result.size();
        absl::StrAppend(&result, strs[0], ", ", stdstrs[1], " ", strs[2], "!");
        EXPECT_EQ(result.substr(old_size), "Hello, std::Cruel World!");

        old_size = result.size();
        absl::StrAppend(&result, pieces[0], ", ", pieces[1], " ", pieces[2]);
        EXPECT_EQ(result.substr(old_size), "Hello, Cruel World");

        old_size = result.size();
        absl::StrAppend(&result, c_strs[0], ", ", c_strs[1], " ", c_strs[2]);
        EXPECT_EQ(result.substr(old_size), "Hello, Cruel World");

        old_size = result.size();
        absl::StrAppend(&result, "ASCII ", i32s[0], ", ", i32s[1], " ", i32s[2], "!");
        EXPECT_EQ(result.substr(old_size), "ASCII 72, 67 87!");

        old_size = result.size();
        absl::StrAppend(&result, ui64s[0], ", ", ui64s[1], "!");
        EXPECT_EQ(result.substr(old_size), "12345678910, 10987654321!");

        std::string one =
            "1";  // Actually, it's the size of this string that we want; a
                  // 64-bit build distinguishes between size_t and uint64_t,
                  // even though they're both unsigned 64-bit values.
        old_size = result.size();
        absl::StrAppend(&result, "And a ", one.size(), " and a ",
                        &result[2] - &result[0], " and a ", one, " 2 3 4", "!");
        EXPECT_EQ(result.substr(old_size), "And a 1 and a 2 and a 1 2 3 4!");

        // result = absl::StrCat("Single chars won't compile", '!');
        // result = absl::StrCat("Neither will nullptrs", nullptr);
        old_size = result.size();
        absl::StrAppend(&result,
                        "To output a char by ASCII/numeric value, use +: ", '!' + 0);
        EXPECT_EQ(result.substr(old_size),
                  "To output a char by ASCII/numeric value, use +: 33");

        // Test 9 arguments, the old maximum
        old_size = result.size();
        absl::StrAppend(&result, 1, 22, 333, 4444, 55555, 666666, 7777777, 88888888,
                        9);
        EXPECT_EQ(result.substr(old_size), "1223334444555556666667777777888888889");

        // No limit thanks to C++11's variadic templates
        old_size = result.size();
        absl::StrAppend(
            &result, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,                           //
            "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",  //
            "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",  //
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",  //
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",  //
            "No limit thanks to C++11's variadic templates");
        EXPECT_EQ(result.substr(old_size),
                  "12345678910abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "No limit thanks to C++11's variadic templates");
    }

    TEST(StrCat, VectorBoolReferenceTypes) {
        std::vector<bool> v;
        v.push_back(true);
        v.push_back(false);
        std::vector<bool> const& cv = v;
        // Test that vector<bool>::reference and vector<bool>::const_reference
        // are handled as if the were really bool types and not the proxy types
        // they really are.
        std::string result = absl::StrCat(v[0], v[1], cv[0], cv[1]); // NOLINT
        EXPECT_EQ(result, "1010");
    }

    // Passing nullptr to memcpy is undefined behavior and this test
    // provides coverage of codepaths that handle empty strings with nullptrs.
    TEST(StrCat, AvoidsMemcpyWithNullptr) {
        EXPECT_EQ(absl::StrCat(42, std::string_view{}), "42");

        // Cover CatPieces code.
        EXPECT_EQ(absl::StrCat(1, 2, 3, 4, 5, std::string_view{}), "12345");

        // Cover AppendPieces.
        std::string result;
        absl::StrAppend(&result, 1, 2, 3, 4, 5, std::string_view{});
        EXPECT_EQ(result, "12345");
    }

//#ifdef GTEST_HAS_DEATH_TEST
//    TEST(StrAppend, Death) {
//        std::string s = "self";
//        // on linux it's "assertion", on mac it's "Assertion",
//        // on chromiumos it's "Assertion ... failed".
//        ABSL_EXPECT_DEBUG_DEATH(absl::StrAppend(&s, s.c_str() + 1),
//                                "ssertion.*failed");
//        ABSL_EXPECT_DEBUG_DEATH(absl::StrAppend(&s, s), "ssertion.*failed");
//    }
//#endif  // GTEST_HAS_DEATH_TEST

    TEST(StrAppend, CornerCases) {
        std::string result;
        absl::StrAppend(&result, "");
        EXPECT_EQ(result, "");
        absl::StrAppend(&result, "", "");
        EXPECT_EQ(result, "");
        absl::StrAppend(&result, "", "", "");
        EXPECT_EQ(result, "");
        absl::StrAppend(&result, "", "", "", "");
        EXPECT_EQ(result, "");
        absl::StrAppend(&result, "", "", "", "", "");
        EXPECT_EQ(result, "");
    }

    TEST(StrAppend, CornerCasesNonEmptyAppend) {
        for(std::string result : {"hello", "a string too long to fit in the SSO"}) {
            const std::string expected = result;
            absl::StrAppend(&result, "");
            EXPECT_EQ(result, expected);
            absl::StrAppend(&result, "", "");
            EXPECT_EQ(result, expected);
            absl::StrAppend(&result, "", "", "");
            EXPECT_EQ(result, expected);
            absl::StrAppend(&result, "", "", "", "");
            EXPECT_EQ(result, expected);
            absl::StrAppend(&result, "", "", "", "", "");
            EXPECT_EQ(result, expected);
        }
    }

    template <typename IntType>
    void CheckHex(IntType v, const char* nopad_format, const char* zeropad_format,
                  const char* spacepad_format) {
        char expected[256];

        std::string actual = absl::StrCat(absl::Hex(v, absl::kNoPad));
        snprintf(expected, sizeof(expected), nopad_format, v);
        EXPECT_EQ(expected, actual) << " decimal value " << v;

        for(int spec = absl::kZeroPad2; spec <= absl::kZeroPad20; ++spec) {
            std::string actual =
                absl::StrCat(absl::Hex(v, static_cast<absl::PadSpec>(spec)));
            snprintf(expected, sizeof(expected), zeropad_format,
                     spec - absl::kZeroPad2 + 2, v);
            EXPECT_EQ(expected, actual) << " decimal value " << v;
        }

        for(int spec = absl::kSpacePad2; spec <= absl::kSpacePad20; ++spec) {
            std::string actual =
                absl::StrCat(absl::Hex(v, static_cast<absl::PadSpec>(spec)));
            snprintf(expected, sizeof(expected), spacepad_format,
                     spec - absl::kSpacePad2 + 2, v);
            EXPECT_EQ(expected, actual) << " decimal value " << v;
        }
    }

    template <typename IntType>
    void CheckDec(IntType v, const char* nopad_format, const char* zeropad_format,
                  const char* spacepad_format) {
        char expected[256];

        std::string actual = absl::StrCat(absl::Dec(v, absl::kNoPad));
        snprintf(expected, sizeof(expected), nopad_format, v);
        EXPECT_EQ(expected, actual) << " decimal value " << v;

        for(int spec = absl::kZeroPad2; spec <= absl::kZeroPad20; ++spec) {
            std::string actual =
                absl::StrCat(absl::Dec(v, static_cast<absl::PadSpec>(spec)));
            snprintf(expected, sizeof(expected), zeropad_format,
                     spec - absl::kZeroPad2 + 2, v);
            EXPECT_EQ(expected, actual)
                << " decimal value " << v << " format '" << zeropad_format
                << "' digits " << (spec - absl::kZeroPad2 + 2);
        }

        for(int spec = absl::kSpacePad2; spec <= absl::kSpacePad20; ++spec) {
            std::string actual =
                absl::StrCat(absl::Dec(v, static_cast<absl::PadSpec>(spec)));
            snprintf(expected, sizeof(expected), spacepad_format,
                     spec - absl::kSpacePad2 + 2, v);
            EXPECT_EQ(expected, actual)
                << " decimal value " << v << " format '" << spacepad_format
                << "' digits " << (spec - absl::kSpacePad2 + 2);
        }
    }

    void CheckHexDec64(uint64_t v) {
        unsigned long long ullv = v;  // NOLINT(runtime/int)

        CheckHex(ullv, "%llx", "%0*llx", "%*llx");
        CheckDec(ullv, "%llu", "%0*llu", "%*llu");

        long long llv = static_cast<long long>(ullv);  // NOLINT(runtime/int)
        CheckDec(llv, "%lld", "%0*lld", "%*lld");

        if(sizeof(v) == sizeof(&v)) {
            auto uintptr = static_cast<uintptr_t>(v);
            void* ptr = reinterpret_cast<void*>(uintptr);
            CheckHex(ptr, "%llx", "%0*llx", "%*llx");
        }
    }

    void CheckHexDec32(uint32_t uv) {
        CheckHex(uv, "%x", "%0*x", "%*x");
        CheckDec(uv, "%u", "%0*u", "%*u");
        int32_t v = static_cast<int32_t>(uv);
        CheckDec(v, "%d", "%0*d", "%*d");

        if(sizeof(v) == sizeof(&v)) {
            auto uintptr = static_cast<uintptr_t>(v);
            void* ptr = reinterpret_cast<void*>(uintptr);
            CheckHex(ptr, "%x", "%0*x", "%*x");
        }
    }

    void CheckAll(uint64_t v) {
        CheckHexDec64(v);
        CheckHexDec32(static_cast<uint32_t>(v));
    }

    void TestFastPrints() {
        // Test all small ints; there aren't many and they're common.
        for(int i = 0; i < 10000; i++) {
            CheckAll(i);
        }

        CheckAll(std::numeric_limits<uint64_t>::max());
        CheckAll(std::numeric_limits<uint64_t>::max() - 1);
        CheckAll(std::numeric_limits<int64_t>::min());
        CheckAll(std::numeric_limits<int64_t>::min() + 1);
        CheckAll(std::numeric_limits<uint32_t>::max());
        CheckAll(std::numeric_limits<uint32_t>::max() - 1);
        CheckAll(std::numeric_limits<int32_t>::min());
        CheckAll(std::numeric_limits<int32_t>::min() + 1);
        CheckAll(999999999);              // fits in 32 bits
        CheckAll(1000000000);             // fits in 32 bits
        CheckAll(9999999999);             // doesn't fit in 32 bits
        CheckAll(10000000000);            // doesn't fit in 32 bits
        CheckAll(999999999999999999);     // fits in signed 64-bit
        CheckAll(9999999999999999999u);   // fits in unsigned 64-bit, but not signed.
        CheckAll(1000000000000000000);    // fits in signed 64-bit
        CheckAll(10000000000000000000u);  // fits in unsigned 64-bit, but not signed.

        CheckAll(999999999876543210);    // check all decimal digits, signed
        CheckAll(9999999999876543210u);  // check all decimal digits, unsigned.
        CheckAll(0x123456789abcdef0);    // check all hex digits
        CheckAll(0x12345678);

        int8_t minus_one_8bit = -1;
        EXPECT_EQ("ff", absl::StrCat(absl::Hex(minus_one_8bit)));

        int16_t minus_one_16bit = -1;
        EXPECT_EQ("ffff", absl::StrCat(absl::Hex(minus_one_16bit)));
    }

    TEST(Numbers, TestFunctionsMovedOverFromNumbersMain) {
        TestFastPrints();
    }
}  // namespace

namespace {
    TEST(MatchTest, StartsWith) {
        const std::string s1("123\0abc", 7);
        const std::string_view a("foobar");
        const std::string_view b(s1);
        const std::string_view e;
        EXPECT_TRUE(absl::StartsWith(a, a));
        EXPECT_TRUE(absl::StartsWith(a, "foo"));
        EXPECT_TRUE(absl::StartsWith(a, e));
        EXPECT_TRUE(absl::StartsWith(b, s1));
        EXPECT_TRUE(absl::StartsWith(b, b));
        EXPECT_TRUE(absl::StartsWith(b, e));
        EXPECT_TRUE(absl::StartsWith(e, ""));
        EXPECT_FALSE(absl::StartsWith(a, b));
        EXPECT_FALSE(absl::StartsWith(b, a));
        EXPECT_FALSE(absl::StartsWith(e, a));
    }

    TEST(MatchTest, EndsWith) {
        const std::string s1("123\0abc", 7);
        const std::string_view a("foobar");
        const std::string_view b(s1);
        const std::string_view e;
        EXPECT_TRUE(absl::EndsWith(a, a));
        EXPECT_TRUE(absl::EndsWith(a, "bar"));
        EXPECT_TRUE(absl::EndsWith(a, e));
        EXPECT_TRUE(absl::EndsWith(b, s1));
        EXPECT_TRUE(absl::EndsWith(b, b));
        EXPECT_TRUE(absl::EndsWith(b, e));
        EXPECT_TRUE(absl::EndsWith(e, ""));
        EXPECT_FALSE(absl::EndsWith(a, b));
        EXPECT_FALSE(absl::EndsWith(b, a));
        EXPECT_FALSE(absl::EndsWith(e, a));
    }

    TEST(MatchTest, Contains) {
        std::string_view a("abcdefg");
        std::string_view b("abcd");
        std::string_view c("efg");
        std::string_view d("gh");
        EXPECT_TRUE(absl::StrContains(a, a));
        EXPECT_TRUE(absl::StrContains(a, b));
        EXPECT_TRUE(absl::StrContains(a, c));
        EXPECT_FALSE(absl::StrContains(a, d));
        EXPECT_TRUE(absl::StrContains("", ""));
        EXPECT_TRUE(absl::StrContains("abc", ""));
        EXPECT_FALSE(absl::StrContains("", "a"));
    }

    TEST(MatchTest, ContainsChar) {
        std::string_view a("abcdefg");
        std::string_view b("abcd");
        EXPECT_TRUE(absl::StrContains(a, 'a'));
        EXPECT_TRUE(absl::StrContains(a, 'b'));
        EXPECT_TRUE(absl::StrContains(a, 'e'));
        EXPECT_FALSE(absl::StrContains(a, 'h'));

        EXPECT_TRUE(absl::StrContains(b, 'a'));
        EXPECT_TRUE(absl::StrContains(b, 'b'));
        EXPECT_FALSE(absl::StrContains(b, 'e'));
        EXPECT_FALSE(absl::StrContains(b, 'h'));

        EXPECT_FALSE(absl::StrContains("", 'a'));
        EXPECT_FALSE(absl::StrContains("", 'a'));
    }

    TEST(MatchTest, ContainsNull) {
        const std::string s = "foo";
        const char* cs = "foo";
        const std::string_view sv("foo");
        const std::string_view sv2("foo\0bar", 4);
        EXPECT_EQ(s, "foo");
        EXPECT_EQ(sv, "foo");
        EXPECT_NE(sv2, "foo");
        EXPECT_TRUE(absl::EndsWith(s, sv));
        EXPECT_TRUE(absl::StartsWith(cs, sv));
        EXPECT_TRUE(absl::StrContains(cs, sv));
        EXPECT_FALSE(absl::StrContains(cs, sv2));
    }

    TEST(MatchTest, EqualsIgnoreCase) {
        std::string text = "the";
        std::string_view data(text);

        EXPECT_TRUE(absl::EqualsIgnoreCase(data, "The"));
        EXPECT_TRUE(absl::EqualsIgnoreCase(data, "THE"));
        EXPECT_TRUE(absl::EqualsIgnoreCase(data, "the"));
        EXPECT_FALSE(absl::EqualsIgnoreCase(data, "Quick"));
        EXPECT_FALSE(absl::EqualsIgnoreCase(data, "then"));
    }

    TEST(MatchTest, StartsWithIgnoreCase) {
        EXPECT_TRUE(absl::StartsWithIgnoreCase("foo", "foo"));
        EXPECT_TRUE(absl::StartsWithIgnoreCase("foo", "Fo"));
        EXPECT_TRUE(absl::StartsWithIgnoreCase("foo", ""));
        EXPECT_FALSE(absl::StartsWithIgnoreCase("foo", "fooo"));
        EXPECT_FALSE(absl::StartsWithIgnoreCase("", "fo"));
    }

    TEST(MatchTest, EndsWithIgnoreCase) {
        EXPECT_TRUE(absl::EndsWithIgnoreCase("foo", "foo"));
        EXPECT_TRUE(absl::EndsWithIgnoreCase("foo", "Oo"));
        EXPECT_TRUE(absl::EndsWithIgnoreCase("foo", ""));
        EXPECT_FALSE(absl::EndsWithIgnoreCase("foo", "fooo"));
        EXPECT_FALSE(absl::EndsWithIgnoreCase("", "fo"));
    }

}  // namespace

int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}