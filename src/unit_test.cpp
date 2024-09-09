#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <iostream>
#include <vector>
#include <numeric>
#include "search_server.h"
#include "unit_test.h"
#include "request_queue.h"
#include "paginator.h"

using namespace std;

template<typename TestFunc>
void RunTestImpl(const TestFunc &func, const string &test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document &doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL_HINT(server.FindTopDocuments("in"s).empty(), false,
                "Стоп-слова должны быть исключены из документов."s);
    }
}

// Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const string content1 = "cat and dog";
    const string content2 = "cat and mouse";
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("cat -dog");
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document &doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id2);
}

void TestAddedDocumentContent() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL,
            { 8, -3 });
    server.AddDocument(1, "ухоженный пёс выразительные глаза"s,
            DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    auto documents = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL_HINT(documents.size(), 2,
            "Добавлены не все документы в поисковый движок."s);
}

void TestFindStatusDocument() {

    const int doc_id1 = 1;
    const int doc_id2 = 2;
    const string content1 = "cat dog";
    const string content2 = "cat mouse";
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::REMOVED, ratings);

    const auto found_docs = server.FindTopDocuments("cat",
            DocumentStatus::REMOVED);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document &doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id2);

}

void TestCalcRating() {

    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL,
            { 8, -3 });
    auto documents = server.FindTopDocuments("пушистый ухоженный кот мышь"s);

    ASSERT_EQUAL_HINT(documents[0].rating, 2, // @suppress("Field cannot be resolved") // @suppress("Invalid arguments")
            "Не правильно считается рейтинг документа."s);
}

void TestCalcRelevationDoc() {

    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL,
            { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
            DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED,
            { 9 });
    auto documents = server.FindTopDocuments("пушистый ухоженный кот"s);

    double EPSILON = 1e-6;
    if (abs(documents[1].relevance - 0.173287) > EPSILON) { // @suppress("Invalid arguments") // @suppress("Field cannot be resolved")

        ASSERT_EQUAL_HINT(documents[1].relevance, 0.173287, // @suppress("Field cannot be resolved") // @suppress("Invalid arguments")
                "Не правильно считается релевантность документа."s);

    }

}

void TestQueue() {
    // Стек вызовов
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s,
            DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s,
            DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s,
            DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s,
            DocumentStatus::ACTUAL, { 1, 1, 1 });
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом

    request_queue.AddFindRequest("big collar"s);
    ASSERT_EQUAL_HINT(request_queue.GetNoResultRequests(), 1438,
            "Не правильно работает очередь запросов. Должно быть 1438 пустых результатов"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    ASSERT_EQUAL_HINT(request_queue.GetNoResultRequests(), 1437,
            "Не правильно работает очередь запросов. Должно быть 1437 пустых результатов"s);

}

void TestPage() {

    // Выводим по страницам
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s,
            DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL,
            { 1, 2, 8 });
    search_server.AddDocument(4, "big dog cat Vladislav"s,
            DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog hamster Borya"s,
            DocumentStatus::ACTUAL, { 1, 1, 1 });
    search_server.AddDocument(6, "big dog hamster Borya"s,
            DocumentStatus::ACTUAL, { 1, 1, 1 });
    search_server.AddDocument(7, "big dog hamster Borya"s,
            DocumentStatus::ACTUAL, { 1, 1, 1 });
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    size_t page_size = 2;
    int page_count = 0;
    const auto pages = Paginate(search_results, page_size); // @suppress("Function cannot be instantiated") // @suppress("Invalid arguments")
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) { // @suppress("Method cannot be resolved")
        ++page_count;
    }

    ASSERT_EQUAL_HINT(page_count, 3,
            "Не правильно работает разделение результата запроса на страницы. Количество возвращаемых страниц должно быть три."s);
}

void TestMatchedMinusWordsDoNotResetPlusWords1() {
    SearchServer server;
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL,
            { 8, -3 });
    server.AddDocument(1, "ухоженный пёс выразительные глаза"s,
            DocumentStatus::BANNED, { 5, -12, 2, 1 });
    auto temp = server.MatchDocument("-кот глаза"s, 1);
    const auto &status = get<1>(temp);
    if (status != DocumentStatus::BANNED)
        ASSERT_EQUAL_HINT(0, 1, "Статус должен вернуться забанен."s);
    const auto &v = get<0>(temp);
    ASSERT_EQUAL_HINT(v[0], "глаза"s, "Возвращаемое слово не совпадает."s);

    auto temp1 = server.MatchDocument("пёс глаза"s, 1);

    const auto &v2 = get<0>(temp1);
    ASSERT_EQUAL_HINT(v2[0], "глаза"s, "Возвращаемое слово не совпадает."s);
    ASSERT_EQUAL_HINT(v2[1], "пёс"s, "Возвращаемое слово не совпадает."s);
}

void TestSortRelevationDoc() {

    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL,
            { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
            DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED,
            { 9 });
    auto documents = server.FindTopDocuments("пушистый ухоженный кот мышь"s);
    ASSERT_EQUAL_HINT(documents[0].id, 1, // @suppress("Invalid arguments") // @suppress("Field cannot be resolved")
            "Не правильная сортировка у документов по релевантности."s);
    ASSERT_EQUAL_HINT(documents[1].id, 0, // @suppress("Invalid arguments") // @suppress("Field cannot be resolved")
            "Не правильная сортировка у документов по релевантности."s);
    ASSERT_EQUAL_HINT(documents[2].id, 2, // @suppress("Invalid arguments") // @suppress("Field cannot be resolved")
            "Не правильная сортировка у документов по релевантности."s);

}

void TestFilterUserPredicateDoc() {

    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL,
            { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
            DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED,
            { 9 });
    auto documents = server.FindTopDocuments("пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) {
                return document_id % 2 == 0;
            });
    ASSERT_EQUAL_HINT(documents[0].id,
            0, // @suppress("Invalid arguments") // @suppress("Field cannot be resolved")
            "Не правильная фильтрация у документов по пользовательскому предикату."s);
    ASSERT_EQUAL_HINT(documents[1].id,
            2, // @suppress("Invalid arguments") // @suppress("Field cannot be resolved")
            "Не правильная фильтрация у документов по пользовательскому предикату."s);

}

/*
 Разместите код остальных тестов здесь
 */

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestAddedDocumentContent);
    RUN_TEST(TestFindStatusDocument);
    RUN_TEST(TestCalcRating);
    RUN_TEST(TestCalcRelevationDoc);
    RUN_TEST(TestSortRelevationDoc);
    RUN_TEST(TestFilterUserPredicateDoc);
    RUN_TEST(TestMatchedMinusWordsDoNotResetPlusWords1);
    RUN_TEST(TestQueue);
    RUN_TEST(TestPage);
}

