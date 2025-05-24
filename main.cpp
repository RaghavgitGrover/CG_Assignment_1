#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <stack>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "file_utils.h"
#include "math_utils.h"
#define GL_SILENCE_DEPRECATION

class MarbleSolitaireGame {
public:
    static const int BOARD_SIZE = 7;
    static constexpr float CELL_SIZE = 0.25f;
    static const int WindowWidth = 800;
    static const int WindowHeight = 600;
    const char* pVSFileName = "shaders/shader.vs";
    const char* pFSFileName = "shaders/shader.fs";

    MarbleSolitaireGame() : initialEmptyRow(3), initialEmptyCol(3), selRow(-1), selCol(-1), stepCounter(0), statusMessage(""), window(nullptr){
        for (auto& row : board) row.fill(0);
    }

    void run() {
        if (!initGLFW()) return;
        glewExperimental = GL_TRUE;
        glewInit();
        printf("GL version: %s\n", glGetString(GL_VERSION));
        InitImGui(window);
        onInit();
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, key_callback);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        while (!glfwWindowShouldClose(window)) {
            glClear(GL_COLOR_BUFFER_BIT);
            onDisplay();
            RenderImGui();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        glfwTerminate();
    }

private:
    std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE> board;
    int initialEmptyRow;
    int initialEmptyCol;
    int selRow;
    int selCol;
    std::stack<std::vector<int>> undoStack;
    std::stack<std::vector<int>> redoStack;
    std::vector<std::pair<int, int>> removedMarbles;
    int stepCounter;
    std::string statusMessage;
    double startTime;

    GLuint squareVAO, squareVBO;
    GLuint circleVAO, circleVBO;
    GLuint gWorldLocation, gColorLocation;

    GLFWwindow* window;

    bool initGLFW() {
        if (!glfwInit()) {
            std::fprintf(stderr, "GLFW initialization failed\n");
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        window = glfwCreateWindow(WindowWidth, WindowHeight, "Marble Solitaire", NULL, NULL);
        if (!window) {
            std::fprintf(stderr, "Failed to create GLFW window\n");
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        return true;
    }

    void InitImGui(GLFWwindow* win) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(win, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    void onInit() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        CreateSquareVertexBuffer();
        CreateCircleVertexBuffer();
        CompileShaders();
        glDisable(GL_DEPTH_TEST);
        initBoard();
        startTime = glfwGetTime();
        statusMessage = "";
    }

    void initBoard() {
        removedMarbles.clear();
        for (std::size_t i = 0; i < board.size(); i++) {
            for (std::size_t j = 0; j < board[i].size(); j++) {
                if (isValidCell(static_cast<int>(i), static_cast<int>(j))) board[i][j] = 1;
                else board[i][j] = -1;
            }
        }
        board[initialEmptyRow][initialEmptyCol] = 0;
        while (!undoStack.empty()) undoStack.pop();
        while (!redoStack.empty()) redoStack.pop();
        undoStack.push(packBoard());
        printMarbles();
    }

    bool isValidCell(int i, int j) {
        if (i < 0 || i >= BOARD_SIZE || j < 0 || j >= BOARD_SIZE) return false;
        if (i == 0 || i == 1 || i == 5 || i == 6)  return (j >= 2 && j <= 4);
        return true;
    }

    std::vector<int> packBoard() {
        std::vector<int> state;
        state.reserve(BOARD_SIZE * BOARD_SIZE);
        for (const auto& row : board) 
            for (const auto& cell : row) 
                state.push_back(cell);
        return state;
    }

    void unpackBoard(const std::vector<int>& state) {
        int idx = 0;
        for (auto& row : board) 
            for (auto& cell : row) 
                cell = state[idx++];
    }

    void printMarbles() {
        stepCounter++;
        int total = 0;
        std::cout << "------------------------------------------\n";
        std::cout << "Step #" << stepCounter << "\nMarbles on Board:\n";
        for (std::size_t i = 0; i < board.size(); i++) {
            for (std::size_t j = 0; j < board[i].size(); j++) {
                if (isValidCell(static_cast<int>(i), static_cast<int>(j)) && board[i][j] == 1) {
                    std::cout << "   (" << i << ", " << j << ")\n";
                    total++;
                }
            }
        }
        std::cout << "Total Marbles: " << total << "\n";
        std::cout << "Removed Marbles: " << removedMarbles.size() << "\n";
        std::cout << "------------------------------------------\n";
    }

    int countMarbles() {
        int cnt = 0;
        for (std::size_t i = 0; i < board.size(); i++) 
            for (std::size_t j = 0; j < board[i].size(); j++) {
                if (isValidCell(static_cast<int>(i), static_cast<int>(j)) && board[i][j] == 1) cnt++;
            }
        return cnt;
    }

    bool noPossibleMoves() {
        static int dRow[4] = {-2, 2, 0, 0};
        static int dCol[4] = {0, 0, -2, 2};
        for (int r = 0; r < BOARD_SIZE; r++) 
            for (int c = 0; c < BOARD_SIZE; c++) {
                if (isValidCell(r, c) && board[r][c] == 1) {
                    for (int k = 0; k < 4; k++) {
                        int rr = r + dRow[k], cc = c + dCol[k];
                        if (isValidCell(rr, cc) && board[rr][cc] == 0) {
                            int jr = r + dRow[k] / 2, jc = c + dCol[k] / 2;
                            if (isValidCell(jr, jc) && board[jr][jc] == 1) return false;
                        }
                    }
                }
            }
        return true;
    }

    bool checkWinCondition() {
        return (noPossibleMoves() && countMarbles() == 1 && board[initialEmptyRow][initialEmptyCol] == 1);
    }

    bool isValidMove(int sr, int sc, int dr, int dc) {
        if (!isValidCell(sr, sc) || !isValidCell(dr, dc)) return false;
        int dRow = dr - sr, dCol = dc - sc;
        if (!((std::abs(dRow) == 2 && dCol == 0) || (std::abs(dCol) == 2 && dRow == 0))) return false;
        if (board[sr][sc] != 1 || board[dr][dc] != 0) return false;
        int jr = sr + dRow / 2, jc = sc + dCol / 2;
        if (!isValidCell(jr, jc) || board[jr][jc] != 1) return false;
        return true;
    }

    void applyMove(int sr, int sc, int dr, int dc) {
        int jr = sr + (dr - sr) / 2, jc = sc + (dc - sc) / 2;
        board[sr][sc] = 0;
        board[jr][jc] = 0;
        board[dr][dc] = 1;
        removedMarbles.push_back(std::make_pair(jr, jc));
        undoStack.push(packBoard());
        while (!redoStack.empty()) redoStack.pop();
        statusMessage = "Move executed.";
        printMarbles();
    }

    void undoMove() {
        if (undoStack.size() <= 1) {
            statusMessage = "No undo available.";
            return;
        }
        std::vector<int> current = undoStack.top();
        undoStack.pop();
        redoStack.push(current);
        unpackBoard(undoStack.top());
        statusMessage = "Undo applied.";
        printMarbles();
    }

    void redoMove() {
        if (redoStack.empty()) {
            statusMessage = "No redo available.";
            return;
        }
        std::vector<int> state = redoStack.top();
        redoStack.pop();
        undoStack.push(state);
        unpackBoard(state);
        statusMessage = "Redo applied.";
        printMarbles();
    }

    void CreateSquareVertexBuffer() {
        float squareVertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f
        };
        glGenVertexArrays(1, &squareVAO);
        glBindVertexArray(squareVAO);
        glGenBuffers(1, &squareVBO);
        glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void CreateCircleVertexBuffer() {
        const int numSegments = 40;
        const float radius = 0.5f;
        std::vector<float> vertices;
        vertices.reserve((numSegments + 2) * 3);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        for (int i = 0; i <= numSegments; i++) {
            float angle = (2.0f * static_cast<float>(M_PI) * i) / numSegments;
            vertices.push_back(radius * std::cos(angle));
            vertices.push_back(radius * std::sin(angle));
            vertices.push_back(0.0f);
        }
        glGenVertexArrays(1, &circleVAO);
        glBindVertexArray(circleVAO);
        glGenBuffers(1, &circleVBO);
        glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void AddShader(GLuint ShaderProgram, const char *pShaderText, GLenum ShaderType) {
        GLuint ShaderObj = glCreateShader(ShaderType);
        if (ShaderObj == 0) {
            std::fprintf(stderr, "Error creating shader type %d\n", ShaderType);
            std::exit(0);
        }
        const GLchar *p[1] = { pShaderText };
        GLint Lengths[1] = { (GLint)std::strlen(pShaderText) };
        glShaderSource(ShaderObj, 1, p, Lengths);
        glCompileShader(ShaderObj);
        GLint success;
        glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar InfoLog[1024];
            glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
            std::fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
            std::exit(1);
        }
        glAttachShader(ShaderProgram, ShaderObj);
    }

    void CompileShaders() {
        GLuint ShaderProgram = glCreateProgram();
        if (ShaderProgram == 0) {
            std::fprintf(stderr, "Error creating shader program\n");
            std::exit(1);
        }
        std::string vs, fs;
        if (!ReadFile(pVSFileName, vs)) std::exit(1);
        if (!ReadFile(pFSFileName, fs)) std::exit(1);
        AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
        AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);
        GLint Success = 0;
        GLchar ErrorLog[1024] = {0};
        glLinkProgram(ShaderProgram);
        glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
        if (Success == 0) {
            glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
            std::fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
            std::exit(1);
        }
        glValidateProgram(ShaderProgram);
        glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
        if (!Success) {
            glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
            std::fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
            std::exit(1);
        }
        glUseProgram(ShaderProgram);
        gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
        gColorLocation = glGetUniformLocation(ShaderProgram, "uColor");
    }

    void renderSquare(const Matrix4f& transform, const Vector4f& color) {
        glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &transform.m[0][0]);
        glUniform4f(gColorLocation, color.x, color.y, color.z, color.w);
        glBindVertexArray(squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    void renderCircle(const Matrix4f& transform, const Vector4f& color) {
        glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &transform.m[0][0]);
        glUniform4f(gColorLocation, color.x, color.y, color.z, color.w);
        glBindVertexArray(circleVAO);
        const int numSegments = 40;
        glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2);
        glBindVertexArray(0);
    }

    void drawBoard() {
        float gridWidth = BOARD_SIZE * CELL_SIZE;
        float gridHeight = BOARD_SIZE * CELL_SIZE;
        float boardScaleFactor = 1.1f;
        float boardWidth = gridWidth * boardScaleFactor;
        float boardHeight = gridHeight * boardScaleFactor;
        Vector4f woodenBoardColor(0.12f, 0.12f, 0.12f, 1.0f);
        Matrix4f boardTrans;
        boardTrans.InitTranslationTransform(0.0f, 0.0f, -0.01f);
        Matrix4f boardScale;
        boardScale.InitScaleTransform(boardWidth, boardHeight, 1.0f);
        Matrix4f worldBoard = boardTrans * boardScale;
        renderSquare(worldBoard, woodenBoardColor);
        float startX = -gridWidth / 2 + CELL_SIZE / 2;
        float startY = gridHeight / 2 - CELL_SIZE / 2;
        Vector4f cupColor(0.12f, 0.12f, 0.12f, 1.0f);
        Vector4f marbleColor(0.9f, 0.9f, 0.9f, 1.0f);
        Vector4f selectedMarbleColor(0.5f, 0.5f, 0.5f, 1.0f);
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (!isValidCell(i, j)) continue;
                float x = startX + j * CELL_SIZE;
                float y = startY - i * CELL_SIZE;
                Matrix4f trans;
                trans.InitTranslationTransform(x, y, 0.0f);
                Matrix4f cupScale;
                cupScale.InitScaleTransform(CELL_SIZE, CELL_SIZE, 1.0f);
                Matrix4f worldCup = trans * cupScale;
                renderCircle(worldCup, cupColor);
                if (board[i][j] == 1) {
                    Matrix4f marbleScale;
                    marbleScale.InitScaleTransform(CELL_SIZE * 0.8f, CELL_SIZE * 0.8f, 1.0f);
                    Matrix4f worldMarble = trans * marbleScale;
                    if (i == selRow && j == selCol) renderCircle(worldMarble, selectedMarbleColor);
                    else renderCircle(worldMarble, marbleColor);
                }
            }
        }
    }

    void drawRemovedMarbles() {
        int count = removedMarbles.size();
        if (count == 0) return;
        float boardWidth = BOARD_SIZE * CELL_SIZE;
        float startX = -boardWidth / 2 + CELL_SIZE / 2;
        float y = -(boardWidth / 2) - CELL_SIZE;
        Vector4f removedColor(0.8f, 0.8f, 0.8f, 1.0f);
        for (int k = 0; k < count; k++) {
            float x = startX + k * (CELL_SIZE * 0.9f);
            Matrix4f trans;
            trans.InitTranslationTransform(x, y, 0.0f);
            Matrix4f scale;
            scale.InitScaleTransform(CELL_SIZE * 0.6f, CELL_SIZE * 0.6f, 1.0f);
            Matrix4f world = trans * scale;
            renderCircle(world, removedColor);
        }
    }

    void onDisplay() {
        glClear(GL_COLOR_BUFFER_BIT);
        drawBoard();
        drawRemovedMarbles();
        GLenum errorCode = glGetError();
        if (errorCode != GL_NO_ERROR) std::fprintf(stderr, "OpenGL rendering error %d\n", errorCode);
    }

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        MarbleSolitaireGame* game = static_cast<MarbleSolitaireGame*>(glfwGetWindowUserPointer(window));
        if (game) game->handleKey(key, scancode, action, mods);
    }

    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        MarbleSolitaireGame* game = static_cast<MarbleSolitaireGame*>(glfwGetWindowUserPointer(window));
        if (game) game->handleMouseButton(button, action, mods);
    }

    void handleKey(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_Q:
                case GLFW_KEY_ESCAPE:
                    glfwSetWindowShouldClose(window, true);
                    break;
                case GLFW_KEY_U:
                    undoMove();
                    break;
                case GLFW_KEY_Y:
                    redoMove();
                    break;
                case GLFW_KEY_R:
                    initBoard();
                    statusMessage = "Board restarted.";
                    break;
                default:
                    break;
            }
        }
    }

    void handleMouseButton(int button, int action, int mods) {
        if (action == GLFW_RELEASE) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            float ndcX = (2.0f * static_cast<float>(xpos)) / WindowWidth - 1.0f;
            float ndcY = 1.0f - (2.0f * static_cast<float>(ypos)) / WindowHeight;
            float boardWidth = BOARD_SIZE * CELL_SIZE;
            float boardHeight = BOARD_SIZE * CELL_SIZE;
            float startX = -boardWidth / 2;
            float startY = boardHeight / 2;
            int col = static_cast<int>((ndcX - startX) / CELL_SIZE);
            int row = static_cast<int>((startY - ndcY) / CELL_SIZE);
            if (!isValidCell(row, col)) return;
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                if (selRow == -1 && selCol == -1) {
                    if (board[row][col] == 1) {
                        selRow = row;
                        selCol = col;
                        statusMessage = "Marble selected.";
                    }
                } 
                else {
                    if (board[row][col] == 1) {
                        selRow = row;
                        selCol = col;
                        statusMessage = "Selection changed.";
                    } 
                    else {
                        int dRow = row - selRow, dCol = col - selCol;
                        if (!((std::abs(dRow) == 2 && dCol == 0) || (std::abs(dCol) == 2 && dRow == 0))) statusMessage = "Invalid move: diagonal jump not allowed.";
                        else if (isValidMove(selRow, selCol, row, col)) applyMove(selRow, selCol, row, col);
                        else statusMessage = "Invalid move.";
                        selRow = selCol = -1;
                    }
                }
            } 
            else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                if (!(row == initialEmptyRow && col == initialEmptyCol)) {
                    initialEmptyRow = row;
                    initialEmptyCol = col;
                    initBoard();
                    statusMessage = "New winning cup set.";
                }
            }
        }
    }

    void RenderImGui() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(610, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(220, 160), ImGuiCond_Always);
        ImGui::Begin("Info", NULL, ImGuiWindowFlags_NoResize);
        double elapsed = glfwGetTime() - startTime;
        ImGui::Text("Time: %.1f s", elapsed);
        ImGui::Text("Remaining: %d", countMarbles());
        ImGui::Text("U=Undo  Y=Redo");
        ImGui::Text("R=Restart  Q=Quit");
        ImGui::Text("L-Click: Select/Move");
        ImGui::Text("R-Click: Set Winning Cup");
        if (noPossibleMoves()) {
            if (checkWinCondition()) ImGui::TextColored(ImVec4(0, 1, 0, 1), "Game Won!");
            else ImGui::TextColored(ImVec4(1, 0, 0, 1), "No moves left!");
        }
        if (!statusMessage.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", statusMessage.c_str());
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

public:
    void renderFrame() {
        onDisplay();
        RenderImGui();
    }
};

int main(int argc, char *argv[]) {
    MarbleSolitaireGame game;
    game.run();
    return 0;
}
