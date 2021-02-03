#ifndef CPARSE_EXTENSIONS_H
#define CPARSE_EXTENSIONS_H

packToken negation(const packToken& left, const packToken& right, evaluationData* data) {
    return !right.asBool();
}

std::string tokenTypeToString(const uint8_t& type) {
    switch (type) {
        case NONE: return "NONE";
        case OP: return "OP";
        case UNARY: return "UNARY";
        case VAR: return "VAR";
        case STR: return "STR";
        case FUNC: return "FUNC";
        case NUM: return "NUM";
        case REAL: return "REAL";
        case INTEGRAL: return "INTEGRAL";
        case BOOLEAN: return "BOOLEAN";
        case IT: return "IT";
        case LIST: return "LIST";
        case TUPLE: return "TUPLE";
        case STUPLE: return "STUPLE";
        case MAP: return "MAP";
        case REF: return "REF";
        case ANY_TYPE: return "ANY_TYPE";
        case TIMER: return "TIMER";
        default: return "UNKNOWN - PLEASE EXTEND THE 'tokenTypeToString' EXTENSION";
    }
}

#endif //CPARSE_EXTENSIONS_H
