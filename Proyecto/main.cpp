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
#include <algorithm> // Para std::find_if

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int TRI_SIZE = 40; // Altura de la fila de triángulos
const float SIDE_PANEL_WIDTH = 200.f;

struct TriCell {
    sf::ConvexShape triangle;
    bool isCrystal = false;
    bool isReflected = false;
    bool isHovered = false;
    bool isExit = false;
    bool isBlocked = false;
    bool isPath = false;
    int row, col;

    TriCell(float x, float y, bool pointingUp, int r, int c) : row(r), col(c) {
        triangle.setPointCount(3);
        // float halfWidth = TRI_SIZE * 0.866f / 2.f; // No se usa directamente aquí si TRI_SIZE es la altura de la fila
        float triWidthOffset = TRI_SIZE / 2.0f; // Ancho desde el centro hasta un vértice de la base, o desde un vértice de la base hasta la punta horizontalmente


        if (pointingUp) { // Base abajo, punta arriba
            triangle.setPoint(0, sf::Vector2f(x, y + TRI_SIZE / 2.f));                     // Izquierda abajo
            triangle.setPoint(1, sf::Vector2f(x + triWidthOffset, y - TRI_SIZE / 2.f));    // Punta arriba
            triangle.setPoint(2, sf::Vector2f(x + triWidthOffset * 2.f, y + TRI_SIZE / 2.f)); // Derecha abajo
        } else { // Base arriba, punta abajo
            triangle.setPoint(0, sf::Vector2f(x, y - TRI_SIZE / 2.f));                     // Izquierda arriba
            triangle.setPoint(1, sf::Vector2f(x + triWidthOffset * 2.f, y - TRI_SIZE / 2.f)); // Derecha arriba
            triangle.setPoint(2, sf::Vector2f(x + triWidthOffset, y + TRI_SIZE / 2.f));    // Punta abajo
        }
        triangle.setFillColor(sf::Color::White);
        triangle.setOutlineThickness(1);
        triangle.setOutlineColor(sf::Color::Black);
    }

    void updateHover(const sf::Vector2f& mousePos) {
        if (triangle.getGlobalBounds().contains(mousePos)) {
            isHovered = true;
            triangle.setFillColor(sf::Color::Yellow);
        } else {
            isHovered = false;
            if (isPath) triangle.setFillColor(sf::Color::Green);
            else if (isBlocked) triangle.setFillColor(sf::Color(50, 50, 50));
            else if (isExit) triangle.setFillColor(sf::Color::Red);
            else if (isCrystal && isReflected) triangle.setFillColor(sf::Color(150, 255, 255));
            else if (isCrystal) triangle.setFillColor(sf::Color::Cyan);
            else triangle.setFillColor(sf::Color::White);
        }
    }
};


void propagateReflection(std::vector<TriCell>& grid, int startRow, int startCol, int rows, int cols) {
    auto getIndex = [&](int r, int c) -> int {
        if (r < 0 || r >= rows || c < 0 || c >= cols) return -1;
        return r * cols + c;
    };

    std::queue<std::pair<int, int>> queue;
    if(getIndex(startRow, startCol) != -1 && grid[getIndex(startRow, startCol)].isCrystal && !grid[getIndex(startRow, startCol)].isReflected) {
        queue.push({startRow, startCol});
    }


    while (!queue.empty()) {
        auto [r, c] = queue.front();
        queue.pop();
        int currentIdx = getIndex(r, c);
        if (currentIdx == -1 || !grid[currentIdx].isCrystal) continue;

        // Direcciones lógicas: {dr, dc}
        // Para reflejar "a través" de la celda (r,c):
        // Si (r+dr, c+dc) es un cristal (el "espejo" o fuente),
        // entonces (r-dr, c-dc) es donde el reflejo aparece.
        // La celda (r,c) es la que está "entre" el espejo y el reflejo.
        // Esto significa que (r,c) debe ser un cristal para que la reflexión ocurra a través de él.
        std::vector<std::pair<int, int>> directions = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        for (auto [dr, dc] : directions) {
            int sourceCrystalIdx = getIndex(r + dr, c + dc); // Celda que podría ser el origen del reflejo
            int reflectionTargetIdx = getIndex(r - dr, c - dc); // Celda donde aparecería el reflejo

            if (sourceCrystalIdx != -1 && reflectionTargetIdx != -1) {
                // Si la celda (r,c) es un cristal (base o reflejo ya existente)
                // Y la celda sourceCrystalIdx es un cristal (base o reflejo)
                // Y la celda reflectionTargetIdx está vacía y puede ser un reflejo
                if (grid[currentIdx].isCrystal && grid[sourceCrystalIdx].isCrystal &&
                    !grid[reflectionTargetIdx].isCrystal && // No es ya un cristal
                    !grid[reflectionTargetIdx].isBlocked &&
                    !grid[reflectionTargetIdx].isExit) {

                    grid[reflectionTargetIdx].isCrystal = true;
                    grid[reflectionTargetIdx].isReflected = true;
                    // Color se actualiza por updateHover
                    queue.push({grid[reflectionTargetIdx].row, grid[reflectionTargetIdx].col}); // Propagar desde el nuevo reflejo
                }
            }
        }
    }
}

// Función para obtener celdas adyacentes que comparten un lado completo
std::vector<std::pair<int, int>> obtenerVecinosAdyacentes(int r, int c, int rows, int cols, bool cellPointingUp) {
    std::vector<std::pair<int, int>> vecinos;

    // Vecinos laterales (comparten lados inclinados)
    // Estos vecinos siempre tendrán la orientación opuesta a la celda (r,c)
    if (c > 0) vecinos.push_back({r, c - 1});          // Izquierda
    if (c < cols - 1) vecinos.push_back({r, c + 1});    // Derecha

    // Tercer vecino (comparte el lado horizontal/base)
    if (cellPointingUp) { // La celda (r,c) apunta hacia arriba (su base está "abajo")
        if (r < rows - 1) vecinos.push_back({r + 1, c}); // El vecino que comparte la base está en (r+1,c) y apuntará hacia abajo
    } else { // La celda (r,c) apunta hacia abajo (su base está "arriba")
        if (r > 0) vecinos.push_back({r - 1, c});    // El vecino que comparte la base está en (r-1,c) y apuntará hacia arriba
    }
    return vecinos;
}


void moverJugador(std::vector<TriCell>& grid, int& jugadorFila, int& jugadorCol, int targetRow, int targetCol, int rows, int cols, int& exitIndex, int& turnCounter, int turnThreshold) {
    // Se asume que targetRow, targetCol es una celda válida y adyacente, y no bloqueada.
    // Esta validación se hace ANTES de llamar a moverJugador.

    int actualIdx = jugadorFila * cols + jugadorCol;

    // Deja un rastro de cristal base en la celda anterior, si no es la salida o bloqueada
    if (actualIdx >=0 && actualIdx < grid.size() && !grid[actualIdx].isExit && !grid[actualIdx].isBlocked) {
        grid[actualIdx].isCrystal = true;
        grid[actualIdx].isReflected = false;
    }

    jugadorFila = targetRow;
    jugadorCol = targetCol;
    int jugadorCurrentIdx = jugadorFila * cols + jugadorCol;

    turnCounter++;

    if (turnCounter >= turnThreshold) {
        // 1. Cambiar la posición de la salida
        if(exitIndex >= 0 && exitIndex < grid.size()) grid[exitIndex].isExit = false;
        
        int newExitIndex;
        do {
            newExitIndex = rand() % grid.size();
        } while (newExitIndex == jugadorCurrentIdx || (newExitIndex < grid.size() && grid[newExitIndex].isBlocked)); // Asegurar que newExitIndex sea válido para grid
        exitIndex = newExitIndex;
        if(exitIndex >= 0 && exitIndex < grid.size()){
            grid[exitIndex].isExit = true;
            grid[exitIndex].isCrystal = false;
            grid[exitIndex].isReflected = false;
        }


        // 2. Colocar 5 obstáculos aleatorios
        int obstaclesPlaced = 0;
        int attempts = 0;
        while(obstaclesPlaced < 5 && attempts < grid.size() * 2 ) {
            int idx = rand() % grid.size();
            if (idx != jugadorCurrentIdx && idx != exitIndex && !grid[idx].isBlocked && !grid[idx].isExit && !grid[idx].isCrystal) {
                grid[idx].isBlocked = true;
                grid[idx].isCrystal = false; 
                grid[idx].isReflected = false;
                obstaclesPlaced++;
            }
            attempts++;
        }

        // 3. Limpiar todos los reflejos existentes.
        for (auto& cell : grid) {
            if (cell.isReflected) {
                cell.isCrystal = false;
                cell.isReflected = false;
            }
            if (cell.isBlocked && cell.isCrystal) { // Si un obstáculo cae sobre un cristal
                cell.isCrystal = false;
            }
        }
        
        // 4. Reiniciar turnCounter
        turnCounter = 0;
    }

    if (jugadorCurrentIdx == exitIndex) {
        std::cout << "\u00a1Has llegado a la salida!\n";
        // TODO: Lógica de victoria/reinicio
    }
}

void exportarEstadoMapa(const std::vector<TriCell>& grid, int rows, int cols) {
    std::ofstream outFile("estado_mapa.txt");
    if (outFile.is_open()) {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int idx = r * cols + c;
                if (idx < 0 || idx >= grid.size()) continue; // Seguridad
                if (grid[idx].isExit) outFile << "S ";
                else if (grid[idx].isPath) outFile << "P ";
                else if (grid[idx].isBlocked) outFile << "X ";
                else if (grid[idx].isCrystal && grid[idx].isReflected) outFile << "R ";
                else if (grid[idx].isCrystal) outFile << "M ";
                else outFile << ". ";
            }
            outFile << "\n";
        }
        outFile.close();
        std::cout << "Mapa exportado a estado_mapa.txt\n";
    } else {
        std::cerr << "Error al abrir estado_mapa.txt para exportar.\n";
    }
}

void animatePath(std::vector<TriCell>& grid, const std::vector<int>& pathIndices, int& pathStep, sf::Clock& animationClock, sf::Time delay) {
    if (pathStep < pathIndices.size() && animationClock.getElapsedTime() > delay) {
        int idx = pathIndices[pathStep];
        if (idx >= 0 && idx < grid.size()) {
            grid[idx].isPath = true;
        }
        animationClock.restart();
        pathStep++;
    }
}

std::vector<int> calcularCaminoAnimado(std::vector<TriCell>& grid, int exitIndex, int rows, int cols, int jugadorR, int jugadorC) {
    auto getIndex = [&](int r, int c) -> int {
        if (r < 0 || r >= rows || c < 0 || c >= cols) return -1;
        return r * cols + c;
    };

    for (auto& cell : grid) cell.isPath = false; // Limpiar caminos anteriores
    std::queue<int> q;
    std::unordered_map<int, int> parent; // K: child_idx, V: parent_idx

    // Iniciar BFS desde el cristal base más cercano al jugador, o el primer cristal base si no hay jugador cerca de uno.
    // Opcionalmente, iniciar desde la posición del jugador si está sobre un cristal.
    int startNodeIdx = -1;
    int jugadorCurrentIdx = getIndex(jugadorR, jugadorC);

    if (jugadorCurrentIdx != -1 && grid[jugadorCurrentIdx].isCrystal && !grid[jugadorCurrentIdx].isBlocked) {
        startNodeIdx = jugadorCurrentIdx;
    } else { // Buscar cualquier cristal base si el jugador no está en uno
        for (int i = 0; i < grid.size(); ++i) {
            if (grid[i].isCrystal && !grid[i].isReflected && !grid[i].isBlocked && !grid[i].isExit) {
                startNodeIdx = i;
                break;
            }
        }
    }

    if (startNodeIdx == -1) return {}; // No hay punto de inicio válido

    q.push(startNodeIdx);
    parent[startNodeIdx] = -1; // Marcar el inicio sin padre

    std::vector<int> path;
    bool foundExit = false;

    while (!q.empty()) {
        int currentIdx = q.front();
        q.pop();

        if (currentIdx == exitIndex) {
            foundExit = true;
            break; // Salida encontrada
        }

        bool currentIsPointingUp = (grid[currentIdx].row + grid[currentIdx].col) % 2 == 0;
        std::vector<std::pair<int, int>> vecinosCoords = obtenerVecinosAdyacentes(grid[currentIdx].row, grid[currentIdx].col, rows, cols, currentIsPointingUp);

        for (const auto& vecinoCoord : vecinosCoords) {
            int neighborIdx = getIndex(vecinoCoord.first, vecinoCoord.second);
            if (neighborIdx != -1 && !grid[neighborIdx].isBlocked && (grid[neighborIdx].isCrystal || neighborIdx == exitIndex) && parent.find(neighborIdx) == parent.end()) {
                parent[neighborIdx] = currentIdx;
                q.push(neighborIdx);
            }
        }
    }

    if (foundExit) {
        int crawl = exitIndex;
        while (crawl != -1) {
            path.insert(path.begin(), crawl);
            if (parent.find(crawl) != parent.end()) {
                crawl = parent[crawl];
            } else {
                break; // Debería ser -1 para el nodo inicial
            }
        }
    }
    return path;
}


int main()
{
    srand(static_cast<unsigned>(time(0)));
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Cuevas de Cristal por Ratón");
    window.setFramerateLimit(60);

    sf::Clock pathClock;
    int pathStep = 0;
    std::vector<int> pathAnimado;

    std::vector<TriCell> grid;
    const int cols = static_cast<int>((WINDOW_WIDTH - SIDE_PANEL_WIDTH) / (TRI_SIZE / 2.0f));
    const int rows = static_cast<int>(WINDOW_HEIGHT / (TRI_SIZE / 2.0f));


    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float x_pos = c * (TRI_SIZE / 2.0f);
            float y_pos = r * (TRI_SIZE / 2.0f) + (TRI_SIZE / 2.f); 
            bool pointingUp = (r + c) % 2 == 0;
            
            // El ajuste original para y_pos en r=0 era redundante si y_pos es el centro del espacio vertical del triángulo.
            // El cálculo de y_pos ya centra los triángulos de la primera fila correctamente.
            // Si se quisiera que el borde superior de la rejilla sea y=0:
            // y_pos para r=0 es TRI_SIZE/2.
            // Si pointingUp, la punta superior está en y_pos - TRI_SIZE/2 = 0.
            // Si !pointingUp, la base superior está en y_pos - TRI_SIZE/2 = 0.
            // Por lo tanto, el ajuste if (r==0) y_pos = TRI_SIZE/2.f; no cambia el valor.

            grid.emplace_back(x_pos, y_pos, pointingUp, r, c);
        }
    }

    int jugadorFila = rand() % rows;
    int jugadorCol = rand() % cols;
    
    sf::CircleShape jugadorMarker(TRI_SIZE / 5); // Marcador un poco más pequeño
    jugadorMarker.setFillColor(sf::Color::Blue);
    jugadorMarker.setOrigin(jugadorMarker.getRadius(), jugadorMarker.getRadius());

    int turnCounter = 0;
    int turnThreshold = 10; 
    int exitIndex = rand() % grid.size();
    // Asegurar que la salida no sea la posición inicial del jugador
    while(grid[exitIndex].row == jugadorFila && grid[exitIndex].col == jugadorCol) {
        exitIndex = rand() % grid.size();
    }
    if(exitIndex >=0 && exitIndex < grid.size()) grid[exitIndex].isExit = true;


    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cerr << "Error: No se pudo cargar la fuente arial.ttf" << std::endl;
        return 1; // Salir si la fuente no carga
    }

    sf::RectangleShape sidePanel(sf::Vector2f(SIDE_PANEL_WIDTH, WINDOW_HEIGHT));
    sidePanel.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH, 0);
    sidePanel.setFillColor(sf::Color(30, 30, 30, 220));

    sf::Text turnText("", font, 18); // Tamaño de fuente ajustado
    turnText.setFillColor(sf::Color::White);
    turnText.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, 20);

    sf::Text crystalText("", font, 16);
    crystalText.setFillColor(sf::Color(200, 255, 255));
    crystalText.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, 50);
    
    float buttonStartY = 90.f;
    float buttonSpacing = 35.f;
    int buttonFontSize = 16;

    sf::Text exportButton("Exportar Mapa", font, buttonFontSize);
    exportButton.setFillColor(sf::Color::Cyan);
    exportButton.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, buttonStartY);

    sf::Text resolveButton("Resolver Camino", font, buttonFontSize);
    resolveButton.setFillColor(sf::Color::Green);
    resolveButton.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, buttonStartY + buttonSpacing);

    sf::Text clearButton("Limpiar Cristales", font, buttonFontSize);
    clearButton.setFillColor(sf::Color::Magenta);
    clearButton.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, buttonStartY + buttonSpacing * 2);

    sf::Text legendTitle("Leyenda:", font, buttonFontSize);
    legendTitle.setFillColor(sf::Color(230,230,230));
    legendTitle.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, buttonStartY + buttonSpacing * 3 + 20);

    sf::Text legendContent(
        "Click Adyacente: Mover\n"
        "Click Celda: Cristal On/Off\n\n"
        "Cian - Cristal Base\n"
        "Azul claro - Reflejo\n"
        "Rojo - Salida\n"
        "Azul - Jugador (marcador)\n"
        "Amarillo - Hover\n"
        "Gris - Bloqueado\n"
        "Verde - Camino Solucion", font, 14);
    legendContent.setFillColor(sf::Color(200,200,200));
    legendContent.setPosition(WINDOW_WIDTH - SIDE_PANEL_WIDTH + 10, buttonStartY + buttonSpacing * 3 + 45);

    bool playerHasMovedOrMapChanged = true; 

    while (window.isOpen())
    {
        sf::Event event;
        
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                    bool actionTakenOnUI = false;

                    // Chequear botones de UI primero
                    if (exportButton.getGlobalBounds().contains(mousePos)) {
                        exportarEstadoMapa(grid, rows, cols);
                        actionTakenOnUI = true;
                    } else if (resolveButton.getGlobalBounds().contains(mousePos)) {
                        pathAnimado = calcularCaminoAnimado(grid, exitIndex, rows, cols, jugadorFila, jugadorCol);
                        pathStep = 0;
                        pathClock.restart();
                        playerHasMovedOrMapChanged = true; 
                        actionTakenOnUI = true;
                    } else if (clearButton.getGlobalBounds().contains(mousePos)) {
                        for (auto &cell : grid) {
                            if (!cell.isExit && !cell.isBlocked) {
                                cell.isCrystal = false;
                                cell.isReflected = false;
                                cell.isPath = false; 
                            }
                        }
                        turnCounter = 0; 
                        playerHasMovedOrMapChanged = true;
                        actionTakenOnUI = true;
                    }

                    if (!actionTakenOnUI && mousePos.x < WINDOW_WIDTH - SIDE_PANEL_WIDTH) { 
                        for (int i = 0; i < grid.size(); ++i) {
                            if (grid[i].triangle.getGlobalBounds().contains(mousePos)) {
                                int clickedRow = grid[i].row;
                                int clickedCol = grid[i].col;
                                bool movedPlayerThisClick = false;

                                // Intentar mover al jugador si la celda clickeada es adyacente y no bloqueada
                                if (!grid[i].isBlocked) {
                                    // Determinar la orientación de la celda actual del jugador
                                    bool playerCellIsPointingUp = (jugadorFila + jugadorCol) % 2 == 0;
                                    std::vector<std::pair<int, int>> vecinos = obtenerVecinosAdyacentes(jugadorFila, jugadorCol, rows, cols, playerCellIsPointingUp);
                                    
                                    for (const auto& vecino : vecinos) {
                                        if (vecino.first == clickedRow && vecino.second == clickedCol) {
                                            // La celda clickeada es un vecino válido y no está bloqueada (ya verificado por !grid[i].isBlocked)
                                            moverJugador(grid, jugadorFila, jugadorCol, clickedRow, clickedCol, rows, cols, exitIndex, turnCounter, turnThreshold);
                                            playerHasMovedOrMapChanged = true;
                                            movedPlayerThisClick = true;
                                            break;
                                        }
                                    }
                                }

                                // Si no se movió al jugador, procesar como clic para colocar/quitar cristal base
                                if (!movedPlayerThisClick) {
                                    if (!grid[i].isExit && !grid[i].isBlocked && !grid[i].isReflected) { 
                                        grid[i].isCrystal = !grid[i].isCrystal; 
                                        
                                        // Limpiar todos los reflejos y repropagar desde todos los cristales base
                                        for(auto& cell_to_clean : grid) {
                                            if (cell_to_clean.isReflected) {
                                                cell_to_clean.isCrystal = false;
                                                cell_to_clean.isReflected = false;
                                            }
                                        }
                                        for(int r_idx = 0; r_idx < rows; ++r_idx) {
                                            for (int c_idx = 0; c_idx < cols; ++c_idx) {
                                                int current_cell_idx = r_idx * cols + c_idx;
                                                if (grid[current_cell_idx].isCrystal && !grid[current_cell_idx].isReflected && !grid[current_cell_idx].isBlocked && !grid[current_cell_idx].isExit) {
                                                    propagateReflection(grid, r_idx, c_idx, rows, cols);
                                                }
                                            }
                                        }
                                        playerHasMovedOrMapChanged = true;
                                    }
                                }
                                break; 
                            }
                        }
                    }
                }
            }
        } 

        if (playerHasMovedOrMapChanged) {
            pathAnimado = calcularCaminoAnimado(grid, exitIndex, rows, cols, jugadorFila, jugadorCol);
            pathStep = 0;
            pathClock.restart();
            playerHasMovedOrMapChanged = false; // Resetear la bandera
        }

        sf::Vector2f currentMousePosView = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        int currentJugadorIdx = jugadorFila * cols + jugadorCol;

        for (int i=0; i< grid.size(); ++i) {
            grid[i].updateHover(currentMousePosView);
        }
        
        animatePath(grid, pathAnimado, pathStep, pathClock, sf::milliseconds(60)); // Animación un poco más rápida

        turnText.setString("Turnos: " + std::to_string(turnCounter) + "/" + std::to_string(turnThreshold));
        long crystalBaseCount = 0;
        for(const auto& cell : grid){
            if(cell.isCrystal && !cell.isReflected && !cell.isBlocked && !cell.isExit) crystalBaseCount++;
        }
        crystalText.setString("Cristales Base: " + std::to_string(crystalBaseCount));


        window.clear(sf::Color(40, 40, 45));
        for (const auto &cell : grid)
            window.draw(cell.triangle);

        window.draw(sidePanel);
        window.draw(turnText);
        window.draw(crystalText);
        window.draw(exportButton);
        window.draw(resolveButton);
        window.draw(clearButton);
        window.draw(legendTitle);
        window.draw(legendContent);
        
        // Posicionar y dibujar el marcador del jugador
        if (currentJugadorIdx >= 0 && currentJugadorIdx < grid.size()) {
            const sf::ConvexShape& playerCellTriangle = grid[currentJugadorIdx].triangle;
            sf::Vector2f playerMarkerPos;
            // Calcular el centroide del triángulo para el marcador
            playerMarkerPos.x = (playerCellTriangle.getPoint(0).x + playerCellTriangle.getPoint(1).x + playerCellTriangle.getPoint(2).x) / 3.f;
            playerMarkerPos.y = (playerCellTriangle.getPoint(0).y + playerCellTriangle.getPoint(1).y + playerCellTriangle.getPoint(2).y) / 3.f;
            jugadorMarker.setPosition(playerMarkerPos);
            window.draw(jugadorMarker);
        }
        
        window.display();
    }

    return 0;
}