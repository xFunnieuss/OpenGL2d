#include <chrono>
#include <cmath>
#include <iostream>
#include <math.h>
#include <thread>
#include <GLFW/glfw3.h>
struct Circle;
struct Block;
//sudo header area
GLFWwindow* init();
void bindLeftClick(GLFWwindow* window);
void drawCircle(GLFWwindow* window, double radius, double posX, double posY, double r, double g, double b);
void drawBlock(GLFWwindow* window, double x, double y, double length, double width, double r, double g, double b);
void cordToNDC(double mousex, double mousey, float* ndcX, float* ndcY);
int mouseUp = 1;
int width = 640;
int lastWidth = 640;
int height = 480;
int lastHeight = 480;
int windowPosX = 0;
int lastWPX = 0;
int windowPosY = 0;
int lastWPY = 0;
double offset = 0;
double lastMouseX = 0;
double lastMouseY = 0;
double forceDampening = 0.999;
float aspect = (float)width / (float)height;
std::vector<Circle> circles;
//TODO: make ball collision accurately calculate resulting force after collision
struct Circle {
    int ballId;
    double radius;
    mutable double posX;
    mutable double posY;
    double r;
    double g;
    double b;
    mutable double xVel;
    mutable double yVel;
    mutable int collisionDebounce = 0;
    void updatePosition() {
        collisionDebounce--;
        yVel = yVel - (0.000981/aspect);
        //xVel = xVel - 0.000981;
        //keeps balls relative to where they are

        posX = (posX + 1) * (double)lastWidth / (double)width - 1.0;
        posY = 1 - (1 - posY) * (double)lastHeight / (double)height;
        radius = radius * (double)lastHeight / (double)height;
        int dWX = windowPosX - lastWPX;
        int dWY = windowPosY - lastWPY;
        double impulseX =  -(double)dWX / (width  ) * 0.05;
        double impulseY = (double)dWY / (height ) * 0.05;
        yVel += impulseY;
        xVel += impulseX;
        posX += (xVel / aspect);
        posY += (yVel / aspect);
        //posY -= forceDampening;
        //collision functions for walls
        if (std::abs(posY) + radius >= 1) {
            yVel = yVel * -1;
            if (posY > 1 - radius) {
                posY = 1 - radius;
            }
            if (posY < -1 + radius) {
                posY = -1 + radius;
            }
        }
        if (std::abs(posX) + radius >= 1) {
            xVel = xVel * -1;
            if (posX > 1 - radius) {
                posX = 1 - radius;
            }
            if (posX < -1 + radius) {
                posX = -1 + radius;
            }
        }
        xVel = xVel * forceDampening;
        yVel = yVel * forceDampening;
        //impuse function (movement of balls when wall moves
    }
};
int main() {
    GLFWwindow* window = init();
    std::srand(std::time(0));
    while (!glfwWindowShouldClose(window)) {
        lastWidth = width;
        lastHeight = height;
        lastWPX = windowPosX;
        lastWPY = windowPosY;
        glfwGetWindowSize(window, &width, &height); //dynamically update viewport as person resizes
        glfwGetWindowPos(window, &windowPosX, &windowPosY);
        glViewport(0, 0, width, height);
        aspect = (float)width / (float)height; //also recompute aspect ratio
        glfwSwapBuffers(window);
        glfwPollEvents(); //looks for inputs
        glClear(GL_COLOR_BUFFER_BIT); //wipes previous frame
        for (auto& c : circles) {
            drawCircle(window, c.radius, c.posX, c.posY, c.r, c.g, c.b);
            c.updatePosition();
            //collision func for other balls
            //distance between two points formula
            for (const auto& c2 : circles) {
                if (c.ballId != c2.ballId) {
                    double distance = std::sqrt(std::pow((c2.posX - c.posX), 2) + std::pow((c2.posY - c.posY), 2));
                    if (distance <= c.radius + c2.radius) {
                        double diffX = -(c2.posX - c.posX)/50;
                        double diffY = c.radius + c2.radius -(c2.posY - c.posY);
                        c.xVel = (c.xVel * -1) + diffX;
                        c.yVel = c.yVel * -1;
                    }
                }
            }
        }
        bindLeftClick(window);
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            for (const auto& c : circles)
                c.yVel = c.yVel - 0.000981;
        }
    }
}
GLFWwindow* init() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    if (!glfwInit()) {
        std::cerr << "window wont open brutal";
        return nullptr;
    }
    GLFWwindow* newWin = glfwCreateWindow(width, height, "BALLS", nullptr, nullptr);
    glfwMakeContextCurrent(newWin);
    return newWin;
}
void bindLeftClick(GLFWwindow* window) {
    int clik = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    double mousex, mousey;
    glfwGetCursorPos(window, &mousex, &mousey);
    float ndcX, ndcY;
    cordToNDC(mousex, mousey, &ndcX, &ndcY);
    double mouseDiffX = (ndcX - lastMouseX);
    double mouseDiffY = ndcY - lastMouseY;
    if (clik && mouseUp) {
        mouseUp = 0;
       // std::cout << "Mouse Down!\n";
        //cords are out of -1 to 1 for drawing purposes
        double rad = 0.1;
        circles.emplace_back(random(), rad, ndcX, ndcY, (double)random()/RAND_MAX,(double)random()/RAND_MAX,(double)random()/RAND_MAX, mouseDiffX, mouseDiffY);
    }
    if (!clik && !mouseUp) {
        mouseUp = 1;
        //std::cout << "Mouse Up!\n";
    }
    lastMouseX = ndcX;
    lastMouseY = ndcY;
}
void drawCircle(GLFWwindow* window, double radius, double posX, double posY, double r, double g, double b) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(r,g,b);
    glVertex2d(posX,posY);
    for (int i = 0; i <= 360; i++) {
        float x = radius/aspect * std::cos(i  * std::numbers::pi / 180.0) + posX;
        float y = radius * std::sin(i * std::numbers::pi / 180.0) + posY;
        glVertex2d(x,y);
        //std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    glEnd();
}
void drawBlock(GLFWwindow* window, double x, double y, double length, double width, double r, double g, double b) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(r,g,b);
    length = length/aspect;
    //first half
    glVertex2d(x,y);
    glVertex2d(x + length,y);
    glVertex2d(x,y + width);
    //second half
    glVertex2d(x + length,y + width);
    glVertex2d(x + length,y);
    glVertex2d(x,y);
    glEnd();

}
void cordToNDC(double mousex, double mousey, float* ndcX, float* ndcY) {
    *ndcX = (mousex / width) * 2.0f - 1.0f;
    *ndcY = 1.0f - (mousey / height) * 2.0f;
}
