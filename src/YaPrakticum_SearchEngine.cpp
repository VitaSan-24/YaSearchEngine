//============================================================================
// Name        : YaPrakticum_SearchEngine.cpp
// Author      : Vitaly Sandalov
// Version     : 0.9
// Copyright   : -
// Description : Search engine
//============================================================================

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <tuple>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

enum class DocumentStatus {
    ACTUAL, IRRELEVANT, BANNED, REMOVED
};

struct Document {
    Document(int in_id, double in_relevance, int in_rating) :
            id(in_id), relevance(in_relevance), rating(in_rating) {
    }
    Document() {
        id = 0;
        relevance = 0.0;
        rating = 0;
    }

    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:

    explicit SearchServer(const string &text_stop_words) : SearchServer(SplitIntoWords(text_stop_words)) {
    }

    template<typename Container>
    explicit SearchServer(const Container &container) {

        for (const string &word : container) {

            if (!IsValidString(word)) {
                throw invalid_argument(
                        "Слово `"s + word + "` имеет запрещенные символы."s);
                vector<string> words;
            }

            const auto &result = find(begin(stop_words_), end(stop_words_),
                    word);
            if (result == stop_words_.end())
                if (!word.empty()) {
                    stop_words_.insert(word);
                }
        }
    }
    int GetDocumentCount() const {
        return document_count_;
    }

    vector<string> SplitIntoWords(const string &text) const {
        vector<string> words;
        string word;
        for (const char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    if (!IsValidString(word)) {
                        throw invalid_argument(
                                "Слово `"s + word
                                        + "` имеет запрещенные символы."s);
                    }
                    words.push_back(word);
                    word.clear();
                }
            } else {
                word += c;
            }
        }

        if (!word.empty()) {
            words.push_back(word);
        }

        return words;
    }

    void AddDocument(int document_id, const string &document,
            DocumentStatus status, const vector<int> &rating) {

        PossibleAddDocument(document_id, document);

        const vector<string> words = SplitIntoWordsNoStop(document);
        int count_words = words.size();
        double frequency_occurrence_word = 1. / count_words;
        for (const string &word : words) {
            word_to_document_freqs_[word][document_id] +=
                    frequency_occurrence_word;
        }
        properties_documents_[document_id] = { ComputeAverageRating(rating),
                status };
        insert_doc_.push_back(document_id);
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string &raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    vector<Document> FindTopDocuments(const string &raw_query,
            DocumentStatus find_status) const {
        return FindTopDocuments(raw_query,
                [&find_status](int document_id, DocumentStatus status,
                        int rating) {
                    return status == find_status;
                });
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string &raw_query,
            Filter filter_fun) const {
        vector<Document> result;
        set<string> query_words;
        Query query;
        ParseQuery(raw_query, query);
        CheckQurey(query);
        result = FindAllDocuments(query, filter_fun);

        auto by_relevance = [](const Document &lhs, const Document &rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            return lhs.relevance > rhs.relevance;
        };

        sort(result.begin(), result.end(), by_relevance);
        if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
            result.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return result;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
            int document_id) const {
        Query query;

        ParseQuery(raw_query, query);
        CheckQurey(query);

        vector<string> v_result;
        DocumentStatus doc_stat = DocumentStatus::ACTUAL;

        auto interator = properties_documents_.find(document_id);
        if (interator != properties_documents_.end()) {
            doc_stat = interator->second.status;
        }

        if (query.minus_words.size() != 0) {
            for (const string &minus_word : query.minus_words) {
                auto map_word = word_to_document_freqs_.find(minus_word);
                if (map_word != word_to_document_freqs_.end()) {
                    auto map_doc_id = map_word->second.find(document_id);
                    if (map_doc_id != map_word->second.end()) {
                        return tuple(v_result, doc_stat);
                    }
                }
            }
        }

        if (query.plus_words.size() != 0) {
            for (const string &plus_word : query.plus_words) {
                auto map_word = word_to_document_freqs_.find(plus_word);
                if (map_word != word_to_document_freqs_.end()) {
                    auto map_doc_id = map_word->second.find(document_id);
                    if (map_doc_id != map_word->second.end()) {
                        v_result.push_back(plus_word);
                    }
                }
            }
        }
        sort(v_result.begin(), v_result.end());
        return tuple(v_result, doc_stat);
    }

    int GetDocumentId(int index) const {

        if (index < 0 || index >= static_cast<int>(insert_doc_.size())) {
            throw out_of_range(
                    "Значение индекса документа выходит за пределы допустимого диапазона."s);
        }

        return insert_doc_.at(index);
    }

private:

    struct DocumentProperties {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        set<string> plus_words;
        vector<string> minus_words;
    };

    vector<int> insert_doc_;
    map<int, DocumentProperties> properties_documents_;
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    set<string> stop_words_;

    DocumentProperties GetPropertiesDocument(const int &id) const {
        DocumentProperties doc_result;
        const auto &interator = properties_documents_.find(id);
        if (interator != properties_documents_.end()) {
            return interator->second;
        }
        return doc_result;
    }

    static int ComputeAverageRating(const vector<int> &ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int sum = accumulate(ratings.begin(), ratings.end(), 0);
        int rating = ratings.size();
        return sum / rating;
    }

    bool IsStopWord(const string &word) const {
        if (stop_words_.size() != 0) {
            return stop_words_.count(word) > 0;
        }
        return false;
    }

    vector<string> SplitIntoWordsNoStop(const string &text) const {
        vector<string> words;

        for (const string &word : SplitIntoWords(text)) {
            if (!IsValidString(word)) {
                                throw invalid_argument(
                                        "Текст `"s + text
                                                + "` содержит запрещенные символы."s);
                            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static bool IsValidString(const string &str) {
        return none_of(str.begin(), str.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    void PossibleAddDocument(int document_id, const string &document) const {
        if (document_id < 0) // id документа не может быть меньше нуля
            throw invalid_argument(
                    "Идентификатор документа `"s + document
                            + "` меньше нуля."s);
        auto result = find(insert_doc_.begin(), insert_doc_.end(), document_id);
        if (result != end(insert_doc_)) { // проверка на добавленные идентификаторы документов
            throw invalid_argument(
                    "Идентификатор документа `"s + to_string(document_id)
                            + "` уже был добавлен."s);
        }
        if (document.empty())
            throw invalid_argument(
                    "Документ с идентификатором `"s + to_string(document_id)
                            + "` пустой."s);  // Документ не может быть пустой
    }

    void ParseQuery(const string &text, Query &query) const {
        if (!text.empty()) {
            for (const string &word : SplitIntoWordsNoStop(text)) {
                if (word[0] != '-')
                    query.plus_words.insert(word);
                else {
                    query.minus_words.push_back(word.substr(1));
                }
            }
        }
    }

    void CheckQurey(Query &query) const {
        for (const string &word : query.minus_words) {
            if (word[0] == '-') {
                throw invalid_argument(
                        "Запрос содержит слово с двумя знаками -"s);
            }
            if (word.size() == 0l) {
                throw invalid_argument("Запрос содержит пустые слова."s);
            }
        }
    }

    double CalcIDF(const string &word) const {
        return log(
                static_cast<double>(document_count_)
                        / static_cast<double>(word_to_document_freqs_.at(word).size()));
    }

    template<typename FilterFun>
    vector<Document> FindAllDocuments(const Query &query,
            FilterFun lambda_func) const {
        vector<Document> matched_documents;
        map<int, double> query_result;

        if (query.plus_words.size() != 0) {
            for (const string &plus_word : query.plus_words) {
                const auto &temp_map = word_to_document_freqs_.find(plus_word);
                if (temp_map != word_to_document_freqs_.end()) {
                    const map<int, double> &temp_set = temp_map->second;
                    double idf = CalcIDF(plus_word);
                    for (auto &element : temp_set) {
                        double &rel = query_result[element.first];
                        rel = rel + idf * element.second;
                    }
                }
            }
            if (query.minus_words.size() != 0) {
                for (const string &minus_word : query.minus_words) {
                    const auto &temp_map = word_to_document_freqs_.find(
                            minus_word);
                    if (temp_map != word_to_document_freqs_.end()) {
                        const map<int, double> &temp_set = temp_map->second;
                        for (auto &element : temp_set) {
                            query_result.erase(element.first);
                        }
                    }
                }
            }
            for (auto &res : query_result) {
                DocumentProperties doc_prop = GetPropertiesDocument(res.first);
                if (lambda_func(res.first, doc_prop.status, doc_prop.rating)) {
                    matched_documents.push_back( { res.first, res.second,
                            doc_prop.rating });
                }
            }
        }
        return matched_documents;
    }
};

template<typename Iterator>
class IteratorRange{
public:
	explicit IteratorRange(const Iterator& p_begin, const Iterator& p_end) {
		begin_ = p_begin;
		end_ = p_end;
		if (p_begin == p_end) {
			size_ = 1;
		}
		else {
			for (Iterator it = p_begin; it != p_end; ++it){
				++size_;
			}
		}
	}


	auto begin() const{
		return begin_;
	}
	auto end() const{
		return end_;
	}
	size_t size() const{
		return size_;
	}

private:
	Iterator begin_;
	Iterator end_;
	size_t size_ = 0;
};

template<typename Iterator>
class Paginator{
public:

	explicit Paginator(const Iterator& p_begin, const Iterator& p_end, size_t page_size)  {

    	for (Iterator it = p_begin; it != p_end; ++it){
    				++size_;
    			}
    	if (size_ == 0) {
    		 throw invalid_argument("Ничего не найдено`"s);
    	}

    	if (page_size == 0){
    		pages_.push_back(IteratorRange(p_begin, p_end));
    		return;
    	}

	}

 	auto begin() const{
 		return pages_.begin();
 	}
 	auto end() const{
 		return pages_.end();
 	}

private:
	vector<IteratorRange<Iterator>> pages_;
	size_t size_ = 0;
};

void PrintDocument(const Document &document) {
    cout << "{ "s << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s << "rating = "s
            << document.rating << " }"s << endl;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

ostream& operator*(ostream& output, const Document& document) {
    output << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
    return output;
}

template <typename Iterator>
ostream& operator<<(ostream& os, const IteratorRange<Iterator>& document_range) {
    for (auto document : document_range) {
        os << *document;
    }
    return os;
}

int main() {

	SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    size_t page_size = 0;
    const auto pages = Paginate(search_results, page_size); // @suppress("Function cannot be instantiated")
    // Выводим найденные документы по страницам
   for (auto page = pages.begin(); page != pages.end(); ++page) { // @suppress("Method cannot be resolved")
       cout << *page << endl;
       cout << "Page break"s << endl;
   }
}
