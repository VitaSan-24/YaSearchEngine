/*
 * document.h
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */

#pragma once

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
