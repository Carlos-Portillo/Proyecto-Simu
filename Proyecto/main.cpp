// cuevas_cristal_sfml.cpp
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <unordered_map>
#include <fstream>
#include <iostream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int TRI_SIZE = 40;

struct TriCell
{
    sf::ConvexShape triangle;
    bool isCrystal = false;
    bool isReflected = false;
    bool isHovered = false;
    bool isExit = false;
    bool isBlocked = false;
    bool isPath = false;
    int row, col;

    TriCell(float x, float y, bool pointingUp, int r, int c) : row(r), col(c)
    {
        triangle.setPointCount(3);
        if (pointingUp)
        {
            triangle.setPoint(0, sf::Vector2f(x, y + TRI_SIZE));
            triangle.setPoint(1, sf::Vector2f(x + TRI_SIZE / 2, y));
            triangle.setPoint(2, sf::Vector2f(x + TRI_SIZE, y + TRI_SIZE));
        }
        else
        {
            triangle.setPoint(0, sf::Vector2f(x, y));
            triangle.setPoint(1, sf::Vector2f(x + TRI_SIZE / 2, y + TRI_SIZE));
            triangle.setPoint(2, sf::Vector2f(x + TRI_SIZE, y));
        }
        triangle.setFillColor(sf::Color::White);
        triangle.setOutlineThickness(1);
        triangle.setOutlineColor(sf::Color::Black);
    }

    void updateHover(const sf::Vector2f &mousePos)
    {
        if (triangle.getGlobalBounds().contains(mousePos))
        {
            isHovered = true;
            triangle.setFillColor(sf::Color::Yellow);
        }
        else
        {
            isHovered = false;
            if (isPath)
                triangle.setFillColor(sf::Color::Green);
            else if (isBlocked)
                triangle.setFillColor(sf::Color(50, 50, 50));
            else if (isExit)
                triangle.setFillColor(sf::Color::Red);
            else if (isCrystal && isReflected)
                triangle.setFillColor(sf::Color(150, 255, 255));
            else if (isCrystal)
                triangle.setFillColor(sf::Color::Cyan);
            else
                triangle.setFillColor(sf::Color::White);
        }
    }
};

void propagateReflection(std::vector<TriCell> &grid, int startRow, int startCol, int rows, int cols)
{
    auto getIndex = [&](int r, int c) -> int
    {
        if (r < 0 || r >= rows || c < 0 || c >= cols)
            return -1;
        return r * cols + c;
    };
    std::queue<std::pair<int, int>> queue;
    queue.push({startRow, startCol});

    while (!queue.empty())
    {
        auto [r, c] = queue.front();
        queue.pop();
        int currentIdx = getIndex(r, c);
        if (currentIdx == -1)
            continue;

        std::vector<std::pair<int, int>> directions = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

        for (auto [dr, dc] : directions)
        {
            int neighborIdx = getIndex(r + dr, c + dc);
            int reflectIdx = getIndex(r - dr, c - dc);
            if (neighborIdx != -1 && reflectIdx != -1)
            {
                if (grid[neighborIdx].isCrystal && !grid[reflectIdx].isCrystal && !grid[reflectIdx].isExit && !grid[reflectIdx].isBlocked)
                {
                    grid[reflectIdx].isCrystal = true;
                    grid[reflectIdx].isReflected = true;
                    grid[reflectIdx].triangle.setFillColor(sf::Color(150, 255, 255));
                    queue.push({r - dr, c - dc});
                }
            }
        }
    }
}

void buscarCaminoBFS(std::vector<TriCell> &grid, int exitIndex, int rows, int cols)
{
    auto getIndex = [&](int r, int c) -> int
    {
        if (r < 0 || r >= rows || c < 0 || c >= cols)
            return -1;
        return r * cols + c;
    };
    for (auto &cell : grid)
        cell.isPath = false;
    std::queue<int> q;
    std::unordered_map<int, int> parent;
    int start = -1, end = exitIndex;

    for (int i = 0; i < grid.size(); ++i)
    {
        if (grid[i].isCrystal && !grid[i].isReflected)
        {
            start = i;
            break;
        }
    }
    if (start == -1)
        return;
    q.push(start);
    parent[start] = -1;

    while (!q.empty())
    {
        int idx = q.front();
        q.pop();
        int r = grid[idx].row, c = grid[idx].col;
        if (idx == end)
            break;
        for (auto [dr, dc] : std::vector<std::pair<int, int>>{{0, 1}, {1, 0}, {0, -1}, {-1, 0}})
        {
            int ni = getIndex(r + dr, c + dc);
            if (ni != -1 && (grid[ni].isCrystal || grid[ni].isExit) && !parent.count(ni))
            {
                q.push(ni);
                parent[ni] = idx;
            }
        }
    }

    int node = end;
    while (parent.count(node) && parent[node] != -1)
    {
        grid[node].isPath = true;
        node = parent[node];
    }
}

void exportarEstadoMapa(const std::vector<TriCell> &grid, int rows, int cols)
{
    std::ofstream outFile("estado_mapa.txt");
    if (outFile.is_open())
    {
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                int idx = r * cols + c;
                if (grid[idx].isExit)
                    outFile << "S ";
                else if (grid[idx].isPath)
                    outFile << "P ";
                else if (grid[idx].isBlocked)
                    outFile << "X ";
                else if (grid[idx].isCrystal && grid[idx].isReflected)
                    outFile << "R ";
                else if (grid[idx].isCrystal)
                    outFile << "M ";
                else
                    outFile << ". ";
            }
            outFile << "\n";
        }
        outFile.close();
    }
}

int main()
{
    srand(static_cast<unsigned>(time(0)));
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Cuevas de Cristal");
    std::vector<TriCell> grid;
    const int cols = WINDOW_WIDTH / TRI_SIZE;
    const int rows = WINDOW_HEIGHT / (TRI_SIZE / 2);

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            bool pointingUp = (row + col) % 2 == 0;
            float x = col * (TRI_SIZE / 2);
            float y = row * (TRI_SIZE / 2);
            grid.emplace_back(x, y, pointingUp, row, col);
        }
    }

    int turnCounter = 0;
    int turnThreshold = 10;
    int exitIndex = rand() % grid.size();
    grid[exitIndex].isExit = true;
    grid[exitIndex].triangle.setFillColor(sf::Color::Red);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf"))
    {
        // Si no se carga la fuente, continuar sin texto
        std::cerr << "Error: No se pudo cargar la fuente arial.ttf" << std::endl;
    }

    // Fondo del panel derecho
    sf::RectangleShape sidePanel(sf::Vector2f(200, WINDOW_HEIGHT));
    sidePanel.setPosition(WINDOW_WIDTH - 200, 0);
    sidePanel.setFillColor(sf::Color(30, 30, 30, 220));

    sf::Text turnText("", font, 24);
    turnText.setFillColor(sf::Color::White);
    turnText.setStyle(sf::Text::Bold);
    turnText.setPosition(WINDOW_WIDTH - 190, 20);

    sf::Text crystalText("", font, 20);
    crystalText.setFillColor(sf::Color(200, 255, 255));
    crystalText.setPosition(WINDOW_WIDTH - 190, 60);

    sf::Text legend("", font, 16);
    legend.setFillColor(sf::Color(230, 230, 230));
    legend.setPosition(WINDOW_WIDTH - 190, 110);
    legend.setString(
        "[E] Exportar\n"
        "[R] Resolver\n"
        "[C] Limpiar\n"
        "\n"
        "Leyenda:\n"
        "Cian - Cristal\n"
        "Azul claro - Reflejo\n"
        "Rojo - Salida\n"
        "Amarillo - Hover\n"
        "Gris - Bloqueado\n"
        "Verde - Camino BFS");

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::E)
                {
                    exportarEstadoMapa(grid, rows, cols);
                }
                else if (event.key.code == sf::Keyboard::R)
                {
                    bool hayCristalManual = false;
                    for (auto &c : grid)
                    {
                        if (c.isCrystal && !c.isReflected)
                        {
                            hayCristalManual = true;
                            break;
                        }
                    }
                    if (!hayCristalManual)
                    {
                        for (auto &c : grid)
                        {
                            if (!c.isBlocked && !c.isExit)
                            {
                                c.isCrystal = true;
                                c.isReflected = false;
                                c.triangle.setFillColor(sf::Color::Cyan);
                                propagateReflection(grid, c.row, c.col, rows, cols);
                                break;
                            }
                        }
                    }
                    buscarCaminoBFS(grid, exitIndex, rows, cols);
                }
                else if (event.key.code == sf::Keyboard::C)
                {
                    for (auto &cell : grid)
                    {
                        if (!cell.isExit && !cell.isBlocked)
                        {
                            cell.isCrystal = false;
                            cell.isReflected = false;
                            cell.isPath = false;
                            cell.triangle.setFillColor(sf::Color::White);
                        }
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonPressed)
            {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                for (auto &cell : grid)
                {
                    if (cell.triangle.getGlobalBounds().contains(mousePos))
                    {
                        if (!cell.isExit && !cell.isBlocked && !(cell.isCrystal && cell.isReflected))
                        {
                            cell.isCrystal = !cell.isCrystal;
                            cell.isReflected = false;
                            cell.triangle.setFillColor(cell.isCrystal ? sf::Color::Cyan : sf::Color::White);
                            ++turnCounter;

                            if (cell.isCrystal)
                            {
                                propagateReflection(grid, cell.row, cell.col, rows, cols);
                            }

                            if (turnCounter >= turnThreshold)
                            {
                                grid[exitIndex].isExit = false;
                                grid[exitIndex].triangle.setFillColor(sf::Color::White);
                                exitIndex = rand() % grid.size();
                                grid[exitIndex].isExit = true;
                                grid[exitIndex].triangle.setFillColor(sf::Color::Red);
                                for (int i = 0; i < 5; ++i)
                                {
                                    int idx = rand() % grid.size();
                                    if (!grid[idx].isExit && !grid[idx].isCrystal)
                                    {
                                        grid[idx].isBlocked = true;
                                        grid[idx].triangle.setFillColor(sf::Color(50, 50, 50));
                                    }
                                }
                                turnCounter = 0;
                            }

                            buscarCaminoBFS(grid, exitIndex, rows, cols);
                        }
                        break;
                    }
                }
            }
        }

        // Actualización del juego
        int crystalCount = 0;
        for (const auto &c : grid)
        {
            if (c.isCrystal)
                crystalCount++;
        }

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        for (auto &cell : grid)
        {
            cell.updateHover(mousePos);
        }

        turnText.setString("Turno: " + std::to_string(turnCounter));
        crystalText.setString("Cristales: " + std::to_string(crystalCount));

        // Renderizado
        window.clear(sf::Color::Black);
        for (const auto &cell : grid)
            window.draw(cell.triangle);

        // Panel y textos
        window.draw(sidePanel);
        window.draw(turnText);
        window.draw(crystalText);
        window.draw(legend);
        window.display();
    }

    return 0;
}