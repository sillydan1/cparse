#ifndef CPARSE_EXTENSIONS_H
#define CPARSE_EXTENSIONS_Hb

packToken negation(const packToken& left, const packToken& right,
                 evaluationData* data) {
    return !right.asBool();
}

#endif //CPARSE_EXTENSIONS_H
