#include <chrono>
#include <cmath>
#include <iomanip>
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
void handleBlockCollision(const Circle &c, const Block &b);
void prepareFrame(GLFWwindow* window);
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
//bools for key values
int hDown = 0;
int gDown = 0;
int xDown = 0;
//used in collision functions (both for balls and for the walls)
double COE = 0.75; //coefficient of restitution, 0 for no elasticity and 1 for perfect elasticity
double COF = 0.99; //coefficient of friction, 0 for instant energy loss and 1 for no friction
double drag = 0.999; //drag in air, not actually accruate to anything
//used to ensure proper rendering across window sizes
float aspect = (float)width / (float)height;
std::vector<Circle> circles;
std::vector<Block> blocks;
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
    void updatePosition() {
       // collisionDebounce--; //antiquated
        yVel = yVel + 0.5;
        //xVel = xVel - 0.000981;
        int dWX = windowPosX - lastWPX;
        int dWY = windowPosY - lastWPY;
        //impulse function (balls gain velocity when window is physically shaken)
        double impulseX =  -(double)dWX / (width  ) * 16;
        double impulseY = -(double)dWY / (height ) * 32;
        if (impulseX != 0 && impulseY != 0) {
            yVel += impulseY;
            xVel += impulseX;
        }
        //keeps balls relative to where they are on window resize (glitchy)
         posX = (posX + 1) * (double)lastWidth / (double)width - 1;
         posY = 1 - (1 - posY) * (double)lastHeight / (double)height;
         posX += xVel;
         posY += yVel;
        //collision functions for walls
        if (posY + radius >= height || posY - radius <= 0 ) {
            yVel = (yVel * -1) * COE;
            if (posY > height - radius) {
                posY = height - radius;
                //in contact with ceiling so we apply friction)
                xVel = xVel * COF;
            }
            if (posY < 0 + radius) {
                posY = 0 + radius;
                //in contact with floor so we apply friction)
                xVel = xVel * COF;
            }
        }
        if (posX + radius >= width || posX - radius <= 0 ) {
            xVel = (xVel * -1) * COE;
            if (posX > width - radius) {
                posX = width - radius;
                //in contact with wall so we apply friction
                yVel = yVel * COF;
            }
            if (posX < 0 + radius) {
                posX = 0 + radius;
                //in contact with wall so we apply friction
                yVel = yVel * COF;
            }
        }
        //slow balls down over time even if in the air
        xVel = xVel * drag;
        yVel = yVel * drag;
    }
};
struct Block {
    int blockId;
    double sizeX;
    double sizeY;
    mutable double posX;
    mutable double posY;
    double r;
    double g;
    double b;
    double mass;
};
struct Corner {
    double x;
    double y;
};
int main() {
    GLFWwindow* window = init();
    std::srand(std::time(0));
    while (!glfwWindowShouldClose(window)) {
        if (lastWidth != width || lastHeight != height) { //prevent x11 physics glitch (maybe bind update to actual frame count?)
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
        prepareFrame(window);
        float ndcX, ndcY;
        //draw blocks first for collision sake
        for (Block& b : blocks) {
            cordToNDC(b.posX, b.posY, &ndcX, &ndcY);
            drawBlock(window, ndcX, ndcY, b.sizeX, b.sizeY, b.r, b.g, b.b);
        }
        for (Circle& c : circles) {
            cordToNDC(c.posX, c.posY, &ndcX, &ndcY);
            drawCircle(window, c.radius, ndcX, ndcY, c.r, c.g, c.b);
            c.updatePosition();
            //collision func for other balls
            //distance between two points formula
            for (int i = 0; i < 25; i++) { //multiple collision rounds per frame
                for (const Circle& c2 : circles) {
                    if (c.ballId < c2.ballId) {
                        handleBallCollision(c, c2);
                    }
                }
            }
            //collision func for walls
            for (const Block& b : blocks) {
                handleBlockCollision(c, b);
            }
        }
        bindLeftClick(window);
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
                circles.emplace_back(random(), rad, mousex, mousey, (double)random()/RAND_MAX,(double)random()/RAND_MAX,(double)random()/RAND_MAX, 40, 0, mass);
            }
        }
        else {
            gDown = 0;
        }
        //spawn in wall
        if (glfwGetKey(window, GLFW_KEY_X)) {
            if (!xDown) {
                xDown = 1;
                double bLength = 300;
                double bWidth = 100;
                blocks.emplace_back(random(), bLength, bWidth, mousex - (bLength/4), mousey+(bWidth/4), (double)random()/RAND_MAX,(double)random()/RAND_MAX,(double)random()/RAND_MAX, bLength*bWidth);
            }
        }
        else {
            xDown = 0;
        }
    }
}
GLFWwindow* init() {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    if (!glfwInit()) {
        std::cerr << "window wont open brutal";
        return nullptr;
    }
    GLFWwindow* newWin = glfwCreateWindow(width, height, "2D Ball Simulation", nullptr, nullptr);
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
    lastMouseY = mousey;float a = 0.0f;
}
void drawCircle(GLFWwindow* window, double radius, double posX, double posY, double r, double g, double b) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(r,g,b);
    glVertex2d(posX,posY);
    for (int i = 0; i <= 360; i++) {
        float x = 2 * radius/width * std::cos(i  * std::numbers::pi / 180.0) + posX;
        float y = 2 * radius/height * std::sin(i * std::numbers::pi / 180.0) + posY;
        glVertex2d(x,y);
    }
    glEnd();
}
void drawBlock(GLFWwindow* window, double x, double y, double bLength, double bWidth, double r, double g, double b) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(r,g,b);
    bLength = bLength/width;
    bWidth = bWidth/height;
    //first half
    glVertex2d(x,y);
    glVertex2d(x + bLength,y);
    glVertex2d(x,y + bWidth);
    //second half
    glVertex2d(x + bLength,y + bWidth);
    glVertex2d(x + bLength,y);
    glVertex2d(x,y);
    glEnd();
}
void handleBallCollision(const Circle &c, const Circle &c2) {
    //thank you https://ericleong.me/research/circle-circle/ for the resulting movement calculations
    //first calculate intersect midpoint so balls can be moved from out of each other
    double distance = std::sqrt(std::pow((c2.posX - c.posX), 2) + std::pow((c2.posY - c.posY), 2));
    if (distance <= c.radius + c2.radius ) {
        //calculate normals before separation
        double nx = (c2.posX - c.posX) / distance;
        double ny = (c2.posY - c.posY) / distance;
        //separate
        double midpointx = (c.posX + c2.posX) / 2;
        double midpointy = (c.posY + c2.posY) / 2;
        c.posX = midpointx + c.radius * -nx;
        c.posY = midpointy + c.radius * -ny;
        c2.posX = midpointx + c2.radius * nx;
        c2.posY = midpointy + c2.radius * ny;
        //now calculate impulse
        double p = 2 * (c.xVel * nx + c.yVel * ny - c2.xVel * nx - c2.yVel * ny) / (c.mass + c2.mass);
        p = p * COE;
        c.xVel = c.xVel - p * c.mass * nx;
        c2.xVel = c2.xVel + p * c2.mass * nx;
        c.yVel = c.yVel - p * c.mass * ny;
        c2.yVel = c2.yVel + p * c2.mass * ny;
    }
}
void handleBlockCollision(const Circle &c, const Block &b) {
    Corner bottomLeft = {b.posX, b.posY};
    Corner topLeft = {b.posX, b.posY - b.sizeY/2};
    Corner bottomRight = {b.posX + b.sizeX/2, b.posY};
    Corner topRight = {b.posX + b.sizeX/2, b.posY- b.sizeY/2};
    if (c.posX + c.radius >= topLeft.x && c.posX - c.radius <= topRight.x && c.posY + c.radius >= topLeft.y && c.posY >= topLeft.y - c.radius  && c.posY <= bottomLeft.y + c.radius ) {
        if (c.posX + c.radius >= topLeft.x || c.posX - c.radius <= topRight.x) {
            c.xVel = (c.xVel * -1) * COE;
        }
        if (c.posY >= topLeft.y - c.radius && c.posY <= bottomLeft.y + c.radius ) {
            c.yVel = (c.yVel * -1) * COE;
            if (c.posY >= topLeft.y - c.radius && std::abs(c.posY - bottomLeft.y) > b.sizeY/2 - c.radius) {
                c.posY = topLeft.y - c.radius;
            }
            else {
                c.posY = bottomLeft.y + c.radius;
            }
        }
    }
}
void cordToNDC(double x, double y, float* ndcX, float* ndcY) {
    *ndcX = (x / width) * 2.0f - 1.0f;
    *ndcY = 1.0f - (y / height) * 2.0f;
}
void prepareFrame(GLFWwindow* window) {
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
}