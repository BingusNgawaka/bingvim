#pragma once

#include <string>

enum Mode{
    NORMAL, INSERT, COMMAND, UNDOTREE
};

enum SyntaxGroup{
    KEYWORD, STRING, COMMENT, NUMBER, FUNCTION, VARIABLE, CLASS
};

template <typename T>
struct Vec2{
    T x {};
    T y {};
    
    Vec2<T> operator-(const Vec2<T>& other) const{
        return {x-other.x, y-other.y};
    }
    Vec2<T> operator+(const Vec2<T>& other) const{
        return {x+other.x, y+other.y};
    }
    void operator-=(const Vec2<T>& other){
        x -= other.x;
        y -= other.y;
    }
    void operator*=(const T t){
        x *= t;
        y *= t;
    }
    void operator/=(const T t){
        x /= t;
        y /= t;
    }
    void operator+=(const Vec2<T>& other){
        x += other.x;
        y += other.y;
    }
    Vec2<T> operator/(const T t) const{
        return {x/t, y/t};
    }
    Vec2<T> operator*(const T t) const{
        return {x*t, y*t};
    }
    bool operator==(const Vec2<T>& other) const{
        return (x==other.x && y==other.y);
    }
    bool operator!=(const Vec2<T>& other) const{
        return (x!=other.x || y!=other.y);
    }

    void print(std::string vecName = "Vec2"){
        printf("%s(%d, %d)", vecName.c_str(), x, y);
    }
};
