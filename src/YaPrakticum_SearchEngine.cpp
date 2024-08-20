//============================================================================
// Name        : YaPrakticum_SearchEngine.cpp
// Author      : Vitaly Sandalov
// Version     : 0.6
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

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

vector<string> SplitIntoWords(const string &text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
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

enum class DocumentStatus {
    ACTUAL, IRRELEVANT, BANNED, REMOVED
};

struct Document {
	Document(int in_id, double in_relevance, int in_rating)
		: id(in_id), relevance(in_relevance), rating(in_rating){}
	Document()
		{
			id = 0;
			relevance = 0.0;
			rating = 0;
		}

    int id;
    double relevance;
    int rating ;
};

class SearchServer {
public:
	SearchServer(const string &text_stop_words){
		 for (const string &word : SplitIntoWords(text_stop_words)) {
		            if (!word.empty()) {
		                stop_words_.insert(word);
		            }
		        }
	}
	template<typename Container>
	SearchServer(const Container& container){
		for (const string& word : container)
		{
			const auto& result = find(begin(stop_words_), end(stop_words_), word) ;
			if (result == stop_words_.end())
			if (!word.empty()){
				stop_words_.insert(word);
			}
		}
	}
    int GetDocumentCount() const {
        return document_count_;
    }

    void AddDocument(int document_id, const string &document,
            DocumentStatus status, const vector<int> &rating) {
        if (!document.empty()) {
            const vector<string> words = SplitIntoWordsNoStop(document);
            int count_words = words.size();
            double frequency_occurrence_word = 1. / count_words;
            for (const string &word : words) {
                word_to_document_freqs_[word][document_id] +=
                        frequency_occurrence_word;
            }
            properties_documents_[document_id] = { ComputeAverageRating(rating),
                    status };
            ++document_count_;
        }
    }

    vector<Document> FindTopDocuments(const string &raw_query) const {
        vector<Document> matched_documents;
        matched_documents = FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string &raw_query, DocumentStatus find_status) const {
        vector<Document> matched_documents;
        matched_documents = FindTopDocuments(raw_query,
                [&find_status](int document_id, DocumentStatus status, int rating) {
                    return status == find_status;
                });
        return matched_documents;
    }

    template<typename Filter>
    vector<Document> FindTopDocuments(const string &raw_query,
            Filter filter_fun) const {
        set<string> query_words;
        Query query;
        ParseQuery(raw_query, query);
        auto matched_documents = FindAllDocuments(query, filter_fun);

        auto by_relevance = [](const Document &lhs, const Document &rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            return lhs.relevance > rhs.relevance;
        };

        sort(matched_documents.begin(), matched_documents.end(), by_relevance);
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
            int document_id) const {
        Query query;
        ParseQuery(raw_query, query);
        vector<string> result;
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
                        return tuple(result, doc_stat);
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
                        result.push_back(plus_word);
                    }
                }
            }
        }
        sort(result.begin(), result.end());
        return tuple(result, doc_stat);
    }

private:

    struct DocumentProperties {
        int rating;
        DocumentStatus status;
    };

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

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
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    void ParseQuery(const string &text, Query &query) const {
        if (!text.empty()) {
            for (const string &word : SplitIntoWordsNoStop(text)) {
                if (word[0] != '-')
                    query.plus_words.insert(word);
                else {
                    query.minus_words.insert(word.substr(1));
                }
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
                    DocumentProperties doc_prop = GetPropertiesDocument(
                            res.first);
                    if (lambda_func(res.first, doc_prop.status,
                            doc_prop.rating)) {
                        matched_documents.push_back( { res.first, res.second,
                                doc_prop.rating });
                    }
                }
        }
        return matched_documents;
    }
};

void PrintDocument(const Document &document) {
    cout << "{ "s << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s << "rating = "s
            << document.rating << " }"s << endl;
}

int main() {
    //SearchServer search_server("и в на"s);
	const vector<string> stop_words_vector = {"и"s, "в"s, "на"s, ""s, "в"s};
	SearchServer search_server(stop_words_vector);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,
            DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,
            DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s,
            DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s,
            DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
    for (const Document &document : search_server.FindTopDocuments(
            "пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document &document : search_server.FindTopDocuments(
            "пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document &document : search_server.FindTopDocuments(
            "пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) {
                return document_id % 2 == 0;
            })) {
        PrintDocument(document);
    }
    return 0;
}
