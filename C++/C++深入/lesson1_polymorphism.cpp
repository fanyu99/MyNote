#include <iostream>

class Shape {
public:
    virtual int area() = 0;
    virtual ~Shape() = default;
};

class Circle : public Shape {
public:
    int area() override {
        return static_cast<int>(3.14 * radius * radius);
    }

    explicit Circle(int r) : radius(r) {}

private:
    int radius;
};

class Rectangle : public Shape {
public:
    int area() override {
        return width * height;
    }

    Rectangle(int w, int h) : width(w), height(h) {}

private:
    int width;
    int height;
};

int main() {
    Shape* pC = new Circle(10);
    Shape* pR = new Rectangle(10, 10);

    std::cout << pC->area() << '\n';
    std::cout << pR->area() << '\n';

    delete pC;
    delete pR;
}


