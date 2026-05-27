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
void handleBallCollision(const Circle &c, const Circle &c2);
int mouseUp = 1;

int width = 640;
int lastWidth = 640;

int height = 480;
int originalHeight = 480;
int lastHeight = 480;

int windowPosX = 0;
int lastWPX = 0;
int windowPosY = 0;
int lastWPY = 0;

double offset = 0;
double mousex, mousey;
double lastMouseX = 0;
double lastMouseY = 0;
double forceDampening = 0.999;

int hDown = 0;
int gDown = 0;

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
    double mass;
    mutable int collisionDebounce = 0;
    void updatePosition() {
       // collisionDebounce--; //antiquated
        yVel = yVel + 0.5;
        //xVel = xVel - 0.000981;
        int dWX = windowPosX - lastWPX;
        int dWY = windowPosY - lastWPY;
        //impulse function (balls gain velocity when window is physically shaken)
        double impulseX =  -(double)dWX / (width  ) * 0.5;
        double impulseY = (double)dWY / (height ) * 0.5;
        if (impulseX != 0 && impulseY != 0) {
            yVel += impulseY;
            xVel += impulseX;
        }
        //keeps balls relative to where they are on window resize (glitchy)
         posX = (posX + 1) * (double)lastWidth / (double)width - 1;
         posY = 1 - (1 - posY) * (double)lastHeight / (double)height;
         posX += xVel;
         posY += yVel;
        //posY -= forceDampening;
        //collision functions for walls
        std::cout << std::abs(posY) << std::endl;
        if (posY + radius >= height || posY - radius <= 0 ) {
            yVel = yVel * -1;
            if (posY > height - radius) {
                posY = height - radius;
            }
            if (posY < 0 + radius) {
                posY = 0 + radius;
            }
        }
        if (posX + radius >= width || posX - radius <= 0 ) {
            xVel = xVel * -1;
            if (posY > width - radius) {
                posX = width - radius;
            }
            if (posX < 0 + radius) {
                posX = 0 + radius;
            }
        }
        //slow balls down over time (should we add real drag?)
        xVel = xVel * forceDampening;
        yVel = yVel * forceDampening;
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
        glfwGetCursorPos(window, &mousex, &mousey);
        glViewport(0, 0, width, height);
        aspect = (float)width / (float)height; //also recompute aspect ratio
        glfwSwapBuffers(window);
        glfwPollEvents(); //looks for inputs
        glClear(GL_COLOR_BUFFER_BIT); //wipes previous frame
        for (auto& c : circles) {
            float ndcX, ndcY;
            cordToNDC(c.posX, c.posY, &ndcX, &ndcY);
            drawCircle(window, c.radius, ndcX, ndcY, c.r, c.g, c.b);
            c.updatePosition();
            //collision func for other balls
            //distance between two points formula
            for (const auto& c2 : circles) {
                if (c.ballId != c2.ballId) {
                    handleBallCollision(c, c2);
                }
            }
        }
        bindLeftClick(window);
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            for (const auto& c : circles)
                c.yVel = c.yVel - 0.000981;
        }
        //spawn in still ball
        if (glfwGetKey(window, GLFW_KEY_H)) {
            if (!hDown) {
                hDown = 1;
                double rad = 20;
                double mass = 3.14159 * (rad * rad); //area
                circles.emplace_back(random(), rad, mousex, mousey, (double)random()/RAND_MAX,(double)random()/RAND_MAX,(double)random()/RAND_MAX, 0, 0, mass);
            }
        }
        else {
            hDown = 0;
        }
        //spawn in left ball
        if (glfwGetKey(window, GLFW_KEY_G)) {
            if (!gDown) {
                gDown = 1;
                double rad = 20;
                double mass = 3.14159 * (rad * rad); //area
                circles.emplace_back(random(), rad, mousex, mousey, (double)random()/RAND_MAX,(double)random()/RAND_MAX,(double)random()/RAND_MAX, 0.5, 0, mass);
            }
        }
        else {
            gDown = 0;
        }
    }
}
GLFWwindow* init() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    if (!glfwInit()) {
        std::cerr << "window wont open brutal";
        return nullptr;
    }
    GLFWwindow* newWin = glfwCreateWindow(width, height, "BALLS", nullptr, nullptr);
    glfwMakeContextCurrent(newWin);
    glfwSwapInterval(1);
    return newWin;
}
void bindLeftClick(GLFWwindow* window) {
    int clik = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    double mouseDiffX = (mousex - lastMouseX);
    double mouseDiffY = mousey - lastMouseY;
    if (clik && mouseUp) {
        mouseUp = 0;
       // std::cout << "Mouse Down!\n";
        //cords are out of -1 to 1 for drawing purposes
        //double rad = 0.1 * ((double)originalHeight/height);
        double rad = 20;
        double mass = 3.14159 * (rad * rad); //area
        circles.emplace_back(random(), rad, mousex, mousey, (double)random()/RAND_MAX,(double)random()/RAND_MAX,(double)random()/RAND_MAX, mouseDiffX, mouseDiffY, mass);
    }
    if (!clik && !mouseUp) {
        mouseUp = 1;
        //std::cout << "Mouse Up!\n";
    }
    lastMouseX = mousex;
    lastMouseY = mousey;
}
void drawCircle(GLFWwindow* window, double radius, double posX, double posY, double r, double g, double b) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(r,g,b);
    glVertex2d(posX,posY);
    for (int i = 0; i <= 360; i++) {
        float x = 2 * radius/width * std::cos(i  * std::numbers::pi / 180.0) + posX;
        float y = 2 * radius/height * std::sin(i * std::numbers::pi / 180.0) + posY;
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
void handleBallCollision(const Circle &c, const Circle &c2) {
    //thank you https://ericleong.me/research/circle-circle/ for the resulting movement calculations
    //first calculate intersect midpoint so balls can be moved from out of each other
    double distance = std::sqrt(std::pow((c2.posX - c.posX), 2) + std::pow((c2.posY - c.posY), 2));
    if (distance <= c.radius + c2.radius ) {
        double midpointx = (c.posX + c2.posX) / 2;
        double midpointy = (c.posY + c2.posY) / 2;
        c.posX = midpointx + c.radius * (c.posX - c2.posX) / distance;
        c.posY = midpointy + c.radius * (c.posY - c2.posY) / distance;
        c2.posX = midpointx + c2.radius * (c2.posX - c.posX) / distance;
        c2.posY = midpointy + c2.radius * (c2.posY - c.posY) / distance;
        //now calculate normals
        double nx = (c2.posX - c.posX) / distance;
        double ny = (c2.posY - c.posY) / distance;
        double p = 2 * (c.xVel * nx + c.yVel * ny - c2.xVel * nx + c2.yVel * ny) / (c.mass + c2.mass);
        c.xVel = c.xVel - p * c.mass * nx;
        c2.xVel = c2.xVel + p * c2.mass * nx;
        c.yVel = c.yVel - p * c.mass * ny;
        c2.yVel = c2.yVel + p * c2.mass * ny;
    }
}
void cordToNDC(double x, double y, float* ndcX, float* ndcY) {
    *ndcX = (x / width) * 2.0f - 1.0f;
    *ndcY = 1.0f - (y / height) * 2.0f;
}
