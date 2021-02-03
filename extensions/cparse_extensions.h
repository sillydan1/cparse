#ifndef CPARSE_EXTENSIONS_H
#define CPARSE_EXTENSIONS_H

packToken negation(const packToken& left, const packToken& right, evaluationData* data) {
    return !right.asBool();
}

std::string tokenTypeToString(const uint8_t& type) {
    switch (type) {
        case tokType_NONE: return "tokType_NONE";
        case tokType_OP: return "tokType_OP";
        case tokType_UNARY: return "tokType_UNARY";
        case tokType_VAR: return "tokType_VAR";
        case tokType_STR: return "tokType_STR";
        case tokType_FUNC: return "tokType_FUNC";
        case tokType_NUM: return "tokType_NUM";
        case tokType_REAL: return "tokType_REAL";
        case tokType_INT: return "tokType_INT";
        case tokType_BOOL: return "tokType_BOOL";
        case tokType_IT: return "tokType_IT";
        case tokType_LIST: return "tokType_LIST";
        case tokType_TUPLE: return "tokType_TUPLE";
        case tokType_STUPLE: return "tokType_STUPLE";
        case tokType_MAP: return "tokType_MAP";
        case tokType_REF: return "tokType_REF";
        case tokType_ANY_TYPE: return "tokType_ANY_TYPE";
        case tokType_TIMER: return "tokType_TIMER";
        default: return "UNKNOWN - PLEASE EXTEND THE 'tokenTypeToString' EXTENSION";
    }
}

#endif //CPARSE_EXTENSIONS_H
