#include <iostream>
#include <cctype>
#include <vector>
#include <queue>
#include <optional>
#include <utility>
#include <stack>
#include <memory>
#include <deque>
#include <fstream>
#include <sstream>
using namespace std;

enum TokenType {
    NUMBER,
    IDENTIFIER,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    DEF,
    EQ,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    COMMA
};

union TokenData {
    const char* id;
    int num;
};

struct Token {
    TokenType type;
    TokenData data;
    int line_no;
    int col_no;

    Token(TokenType _type) : type{_type}, data{0},
        line_no{0}, col_no{0} {}

    Token(TokenType _type, int _line_no, int _col_no) : type{_type}, data{0},
        line_no{_line_no}, col_no{_col_no} {}

    Token(TokenType _type, const char* _data, int _line_no, int _col_no) : 
        type{_type}, line_no{_line_no}, col_no{_col_no} {
        data.id = _data;
    }
    Token(TokenType _type, int _data, int _line_no, int _col_no) : 
        type{_type}, line_no{_line_no}, col_no{_col_no} {
        data.num = _data;
    }

    string to_string() {
        string str = "";
        switch(type) {
            case NUMBER: str = "NUMBER"; break;
            case IDENTIFIER: str = "IDENTIFIER"; break;
            case LBRACKET: str = "LBRACKET"; break;
            case RBRACKET: str = "RBRACKET"; break;
            case SEMICOLON: str = "SEMICOLON"; break;
            case DEF: str = "DEF"; break;
            case EQ: str = "EQ"; break;
            case PLUS: str = "PLUS"; break;
            case MINUS: str = "MINUS"; break;
            case STAR: str = "STAR"; break;
            case SLASH: str = "SLASH"; break;
            case COMMA: str = "COMMA"; break;
        }
        if (type == NUMBER) str += " " + std::to_string(data.num);
        if (type == IDENTIFIER) str += " " + string(data.id);
        return str;
    }
};

enum Operator {
    ADD, SUB, MUL, DIV
};

struct ASTNode {};

struct Expression : ASTNode {};

struct NullExpression : Expression {};

struct BinaryExpression : Expression {
    unique_ptr<Expression> lhs;
    Operator op;
    unique_ptr<Expression> rhs;

    BinaryExpression(unique_ptr<Expression>& _lhs, Operator _op, unique_ptr<Expression>& _rhs):
        lhs{std::move(_lhs)},
        op{_op},
        rhs{std::move(_rhs)} {}

};

struct IntLiteral : Expression {
    int value;

    IntLiteral(int _value) : value(_value) {}
};

struct Identifier : Expression {
    string value;

    Identifier(string _value) : value(_value) {}
};

struct FunctionApplicationExpression : Expression {
    string name;
    vector<unique_ptr<Expression>> parameters;

    FunctionApplicationExpression() {}
};

struct FunctionDefinition : ASTNode {
    string name;
    vector<string> params;
    unique_ptr<Expression> value;

    FunctionDefinition(string _name, vector<string> _params, unique_ptr<Expression>& _value):
        name(_name),
        params(_params),
        value(std::move(_value)) {}

    FunctionDefinition() {}
};

std::queue<Token> tokens;
FunctionDefinition curr_fd;
std::queue<unique_ptr<Expression>> expr_queue;
std::vector<FunctionDefinition> definitions;

int tokenize(const string& input) {

    string id;
    bool tok_is_number = false;
    bool in_tok = false;

    bool in_expr = false;
    bool in_func_def = false;
    int line_no = 1;
    int col_no = 0;
    for (int i=0; i<input.length(); i++) {
        char c = input[i];
        if (c == '\t') col_no += 4;
        else col_no++;
        
        if (isnumber(c)) {
            if (in_tok && !tok_is_number) return -1;
            in_tok = true;
            tok_is_number = true;
            id += c;
        }
        else if (isalpha(c)) {
            if (in_tok && tok_is_number) return -1;
            in_tok = true;
            tok_is_number = false;
            id += c;
        }
        else {
            if (in_tok) {
                if (id == "def") {
                    tokens.emplace(DEF, line_no, col_no);
                }
                else if (tok_is_number) {
                    tokens.emplace(NUMBER, stoi(id), line_no, col_no);
                }
                else {
                    tokens.emplace(IDENTIFIER, id.c_str(), line_no, col_no);
                }
            }
            in_tok = false;
            tok_is_number = false;
            id = "";
            if (isspace(c)) {
                if (c == '\n') {
                    line_no++;
                    col_no = 0;
                }
                continue;
            }
            switch (c) {
                case '(': tokens.emplace(LBRACKET, line_no, col_no); break;
                case ')': tokens.emplace(RBRACKET, line_no, col_no); break;
                case '=': tokens.emplace(EQ, line_no, col_no); break;
                case '+': tokens.emplace(PLUS, line_no, col_no); break;
                case '-': tokens.emplace(MINUS, line_no, col_no); break;
                case ',': tokens.emplace(COMMA, line_no, col_no); break;
                case '*': tokens.emplace(STAR, line_no, col_no); break;
                case '/': tokens.emplace(SLASH, line_no, col_no); break;
                case ';': tokens.emplace(SEMICOLON, line_no, col_no); break;
                default : return -1;
            }
        }
    }

    return 0;
}

bool err_stack_empty() {
    cout << "Parse Error: Unexpected EOF" << endl;
    return false;
}

bool err_expr_queue_empty() {
    cout << "Parse Error: Expression queue empty" << endl;
    return false;
}

bool err_token_mismatch(Token expected_tok) {
    cout << "Parse Error: Expected token " << expected_tok.to_string() << 
        ", got " << tokens.front().to_string() << " at line " << 
        tokens.front().line_no << ", col " << tokens.front().col_no;
    return false;
}

bool err_msg(string err) {
    if (!tokens.empty()) {
        cout << "Parse Error: " << err << " at line " << 
            tokens.front().line_no << ", col " << tokens.front().col_no;
    }
    else {
        cout << "Parse Error: " << err << " at EOF";
    }
    return false;
}

bool empty() {
    return tokens.empty();
}

optional<Token> peek() {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    return tokens.front();
}

optional<Token> pop() {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    Token t = tokens.front();
    tokens.pop();
    return t;
}

optional<unique_ptr<Expression>> pop_expr() {
    if (expr_queue.empty()) {
        err_expr_queue_empty();
        return {};
    }
    unique_ptr<Expression> e = std::move(expr_queue.front());
    expr_queue.pop();
    return e;
}

optional<Token> popif(TokenType toktype) {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    if (tokens.front().type != toktype) {
        return {};
    }
    Token t = tokens.front();
    tokens.pop();
    return t;
}

optional<Token> peekif(TokenType toktype) {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    if (tokens.front().type != toktype) {
        return {};
    }
    return tokens.front();
}

bool parse_expr();

bool parse_func_call_expr() {

    if (empty()) return err_stack_empty();

    if (peekif(LBRACKET)) {
        pop();
        if (!parse_expr()) return false;
        if (!popif(RBRACKET)) return err_token_mismatch(Token(RBRACKET));
    }
    else if (peekif(IDENTIFIER)) {
        Token name = pop().value();

        if (peekif(LBRACKET)) {
            pop();
            unique_ptr<FunctionApplicationExpression> fae = make_unique<FunctionApplicationExpression>();
            fae->name = name.data.id;
            while (!popif(RBRACKET)) {
                if (!parse_expr()) return false;
                fae->parameters.push_back(std::move(expr_queue.front()));
                expr_queue.pop();

                if (!(popif(COMMA) || peekif(RBRACKET))) {
                    return err_msg("Expected token , or ), got " + peek()->to_string());
                }
            }
            expr_queue.push(std::move(fae));
        }
        else {
            expr_queue.push(make_unique<Identifier>(name.data.id));
        }
    }
    else if (peekif(NUMBER)) {
        expr_queue.push(make_unique<IntLiteral>(pop().value().data.num));
    }
    else {
        return err_msg("Expected number, identifier or (, got " + peek()->to_string());
    }
    return true;
}

bool parse_mul_div_expr() {

    if (!parse_func_call_expr()) return false;

    if (empty()) return err_stack_empty();

    if (peekif(RBRACKET) || peekif(SEMICOLON) || peekif(COMMA) || peekif(PLUS) || peekif(MINUS)) {
        // do nothing
    }
    else if (peekif(STAR) || peekif(SLASH)) {
        Operator op = pop().value().type == STAR ? MUL : DIV;
        if (!parse_expr()) return false;
        unique_ptr<Expression> e1 = pop_expr().value();
        unique_ptr<Expression> e2 = pop_expr().value();
        expr_queue.push(make_unique<BinaryExpression>(e1, op, e2));
    }
    else {
        return err_msg("Expected ),;,,,+,-,*,/, got " + peek()->to_string());
    }

    return true;
}

bool parse_expr() {

    if (!parse_mul_div_expr()) return false;

    if (empty()) return err_stack_empty();

    if (peekif(RBRACKET) || peekif(SEMICOLON) || peekif(COMMA)) {
        // do nothing
    }
    else if (peekif(PLUS) || peekif(MINUS)) {
        Operator op = pop().value().type == PLUS ? ADD : SUB;
        if (!parse_expr()) return false;
        unique_ptr<Expression> e1 = pop_expr().value();
        unique_ptr<Expression> e2 = pop_expr().value();
        expr_queue.push(make_unique<BinaryExpression>(e1, op, e2));
    }
    else {
        return err_msg("Expected ),;,,,+,-, got " + peek()->to_string());
    }

    return true;
}

bool parse_func_defn() {
    if (!peekif(IDENTIFIER)) return err_token_mismatch(Token(IDENTIFIER));
    curr_fd.name = pop().value().data.id;
    if (!popif(LBRACKET)) return err_token_mismatch(Token(LBRACKET));
    while (!popif(RBRACKET)) {
        if (!peekif(IDENTIFIER)) return err_token_mismatch(Token(IDENTIFIER));
        curr_fd.params.push_back(pop().value().data.id);
        if (!(popif(COMMA) || peekif(RBRACKET))) {
            return err_msg("Expected token , or ), got " + peek()->to_string());
        }
    }
    return true;
}

bool parse() {

    while (!empty()) {
        if (!popif(DEF)) return err_token_mismatch(Token(DEF));
        if (!parse_func_defn()) return false;
        if (!popif(EQ)) return err_token_mismatch(Token(EQ));
        if (!parse_expr()) return false;
        if (!popif(SEMICOLON)) return err_token_mismatch(Token(SEMICOLON));

        unique_ptr<Expression> expr = pop_expr().value();
        definitions.emplace_back(curr_fd.name, curr_fd.params, expr);
        curr_fd.params.clear();
    }
    return true;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        cout << "Usage: formula <filename>" << endl;
    }

    std::ifstream t(argv[1]);
    std::stringstream buffer;
    buffer << t.rdbuf();

    tokenize(buffer.str());
    cout << parse() << endl;

}
