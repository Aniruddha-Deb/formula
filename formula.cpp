#include <iostream>
#include <cctype>
#include <vector>
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

    Token(TokenType _type) : type{_type}, data{0} {}
    Token(TokenType _type, const char* _data) : type{_type} {
        data.id = _data;
    }
    Token(TokenType _type, int _data) : type{_type} {
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

std::vector<Token> tokens;
FunctionDefinition curr_fd;
std::stack<unique_ptr<Expression>> expr_stack;
std::vector<FunctionDefinition> definitions;

int tokenize(const string& input) {

    string id;
    bool tok_is_number = false;
    bool in_tok = false;

    bool in_expr = false;
    bool in_func_def = false;
    for (int i=0; i<input.length(); i++) {
        char c = input[i];
        
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
                    tokens.emplace_back(DEF);
                }
                else if (tok_is_number) {
                    tokens.emplace_back(NUMBER, stoi(id));
                }
                else {
                    tokens.emplace_back(IDENTIFIER, id.c_str());
                }
            }
            in_tok = false;
            tok_is_number = false;
            id = "";
            if (isspace(c)) continue;
            switch (c) {
                case '(': tokens.emplace_back(LBRACKET); break;
                case ')': tokens.emplace_back(RBRACKET); break;
                case '=': tokens.emplace_back(EQ); break;
                case '+': tokens.emplace_back(PLUS); break;
                case '-': tokens.emplace_back(MINUS); break;
                case ',': tokens.emplace_back(COMMA); break;
                case '*': tokens.emplace_back(STAR); break;
                case '/': tokens.emplace_back(SLASH); break;
                case ';': tokens.emplace_back(SEMICOLON); break;
                default : return -1;
            }
        }
    }

    return 0;
}

bool has_next_token(int pos) {
    if (pos+1 >= tokens.size()) {
        cout << "Parse Error: Unexpected end" << endl;
        return false;
    }
    return true;
}

bool assert_type(int pos, TokenType token) {
    if (tokens[pos].type != token) {
        cout << "Parse error: Expected token of type " << " at pos " << pos << endl;
        return false;
    }
    return true;
}

int parse_expr(int pos);

int parse_func_call_expr(int pos) {

    if (tokens[pos].type == LBRACKET) {
        if (!has_next_token(pos)) return -1;
        int next_tok = parse_expr(pos+1);
        if (!assert_type(next_tok, RBRACKET)) return -1;
        if (!has_next_token(next_tok)) return -1;
        return next_tok+1;
    }
    else if (tokens[pos].type == IDENTIFIER) {
        // identify if it's a function call
        if (!has_next_token(pos)) return -1;
        if (tokens[pos+1].type == LBRACKET) {
            // function call 
            std::cout << "function call" << std::endl;
            unique_ptr<FunctionApplicationExpression> fae = make_unique<FunctionApplicationExpression>();
            fae->name = tokens[pos].data.id;
            if (!has_next_token(pos+1)) return -1;
            int i = pos+2;
            while (i < tokens.size() && tokens[i].type != RBRACKET) {
                i = parse_expr(i);
                fae->parameters.push_back(std::move(expr_stack.top()));
                expr_stack.pop();
                if (!assert_type(i, COMMA) || !assert_type(i, RBRACKET)) return -1;
                if (tokens[i].type == COMMA) i++;
            }
            expr_stack.push(std::move(fae));
            if (!has_next_token(i)) return -1;
            std::cout << "parsed function call" << std::endl;
            return i+1;
        }
        else {
            expr_stack.push(make_unique<Identifier>(tokens[pos].data.id));
            return pos+1;
        }
    }
    else if (tokens[pos].type == NUMBER) {
        expr_stack.push(make_unique<IntLiteral>(tokens[pos].data.num));
        if (!has_next_token(pos)) return -1;
        return pos+1;
    }
    cout << "Parse error: Expected func_expression" << endl;
    return -1;
}

int parse_mul_div_expr(int pos) {

    int next_tok = parse_func_call_expr(pos);

    if (tokens[next_tok].type == RBRACKET || tokens[next_tok].type == SEMICOLON ||
        tokens[next_tok].type == COMMA || tokens[next_tok].type == PLUS || tokens[next_tok].type == MINUS) {
        return next_tok;
    }
    else if (tokens[next_tok].type == STAR || tokens[next_tok].type == SLASH) {
        Operator op = tokens[next_tok].type == STAR ? MUL : DIV;
        if (!has_next_token(next_tok)) return -1;
        next_tok = parse_expr(next_tok+1);
        auto e1 = std::move(expr_stack.top());
        expr_stack.pop();
        auto e2 = std::move(expr_stack.top());
        expr_stack.pop();
        expr_stack.push(make_unique<BinaryExpression>(e1, op, e2));
        return next_tok;
    }

    cout << "Parse error: Expected mul_div_expression" << endl;
    return -1;

}

int parse_expr(int pos) {

    int next_tok = parse_mul_div_expr(pos);
    std::cout << next_tok << endl;
    if (tokens[next_tok].type == RBRACKET || tokens[next_tok].type == SEMICOLON || tokens[next_tok].type == COMMA) {
        std::cout << next_tok << " is a semicolon, returning " << endl;
        return next_tok;
    }
    else if (tokens[next_tok].type == PLUS || tokens[next_tok].type == MINUS) {
        Operator op = tokens[next_tok].type == PLUS ? ADD : SUB;
        if (!has_next_token(next_tok)) return -1;
        next_tok = parse_expr(next_tok+1);
        auto e1 = std::move(expr_stack.top());
        expr_stack.pop();
        auto e2 = std::move(expr_stack.top());
        expr_stack.pop();
        expr_stack.push(make_unique<BinaryExpression>(e1, op, e2));
        return next_tok;
    }

    cout << "Parse error: Expected expression, but got token " << tokens[next_tok].to_string() << " at pos " << next_tok << endl;
    return -1;
}

int parse_func_defn(int pos) {
    cout << "Parsing fn defn at " << pos << endl;
    if (!assert_type(pos, IDENTIFIER)) return -1;
    curr_fd.name = tokens[pos].data.id;
    if (!has_next_token(pos)) return -1;
    if (!assert_type(pos+1, LBRACKET)) return -1;
    if (!has_next_token(pos+1)) return -1;
    int i = pos+2;
    std::cout << "Pushed back param " << endl;
    while (i < tokens.size() && tokens[i].type != RBRACKET) {
        if (!assert_type(i, IDENTIFIER)) return -1;
        curr_fd.params.push_back(tokens[i].data.id);
        if (!has_next_token(i)) return -1;
        i++;
        if (tokens[i].type == RBRACKET) continue;
        if (tokens[i].type == COMMA) i++;
        else {
            cout << "Parse error: expected comma" << endl;
            return -1;
        }
    }
    std::cout << "Pushed back param " << endl;
    if (!has_next_token(i)) return -1;
    return i+1;
}

int parse() {

    int i = 0;
    while (i < tokens.size()) {
        if (tokens[i].type != DEF) {
            cout << "Parse Error: expected def token" << endl;
            return -1;
        }
        if (!has_next_token(i)) return -1;
        i = parse_func_defn(i+1);
        if (i == -1) return -1;
        if (tokens[i].type != EQ) {
            cout << "Parse Error: expected equalto token" << endl;
            return -1;
        }
        cout << "Got equalto " << endl;
        if (!has_next_token(i)) return -1;
        i = parse_expr(i+1);
        std::cout << i << endl;
        if (i == -1) return -1;
        if (tokens[i].type != SEMICOLON) {
            cout << "Parse Error: expected semicolon token at pos " << i << endl;
            return -1;
        }

        definitions.emplace_back(curr_fd.name, curr_fd.params, expr_stack.top());
        curr_fd.params.clear();
        expr_stack.pop();
        i++;
    }

    return i;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        cout << "Usage: formula <filename>" << endl;
    }

    std::ifstream t(argv[1]);
    std::stringstream buffer;
    buffer << t.rdbuf();

    tokenize(buffer.str());
    for (auto t : tokens) {
        std::cout << t.to_string() << std::endl;
    }
    cout << parse() << endl;

}
