#pragma once

#include <string.h>

#define RUN_TEST(func) RunTestImpl(func, #func);
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s);
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint));

template<typename TestFunc>
void RunTestImpl(const TestFunc &func, const std::string &test_name);

template<typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const std::string &t_str,
        const std::string &u_str, const std::string &file,
        const std::string &func, unsigned line, const std::string &hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "(" << line << "): " << func << ": ";
        std::cout << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cout << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cout << " Hint: " << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent();
// Поддержка минус-слов. Документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestExcludeMinusWordsFromAddedDocumentContent();
// Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddedDocumentContent();
// Поиск документов, имеющих заданный статус.
void TestFindStatusDocument();
// Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestCalcRating();
// Корректное вычисление релевантности найденных документов.
void TestCalcRelevationDoc();
///Соответствие документов поисковому запросу.
///При этом должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchedMinusWordsDoNotResetPlusWords1();
//Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortRelevationDoc();
// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestFilterUserPredicateDoc();
// Тестирование вывода результата по страницам
void TestPage();
// Тестирование очереди запросов
void TestQueue();

/*
 Разместите код остальных тестов здесь
 */

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();

