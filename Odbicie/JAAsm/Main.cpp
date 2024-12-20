//Anastasiia Pashko
#define NOMINMAX
#include <windows.h>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

typedef void(_stdcall* Odbicie)(volatile uint8_t*, uint32_t, uint32_t, uint32_t); // Typ funkcji do odbicia
typedef void(_stdcall* OdbicieASM)(volatile uint8_t*, uint32_t, uint32_t, uint32_t, uint32_t); // Typ funkcji z biblioteki asemblera
typedef uint64_t(_stdcall* CzasASM)(); // Typ funkcji do pomiaru czasu

HINSTANCE dllHandleCpp = NULL;
HINSTANCE dllHandleAsm = NULL;

void Obrazek_na_Bitmap(sf::Image image, volatile uint8_t* bitmap) {
    int pointer = 0;
    for (uint32_t i = 0; i < image.getSize().x * image.getSize().y * 4; i++) {
        if (i % 4 == 3) {
            continue; // Pomijanie kanału alfa
        }
        bitmap[pointer] = (image.getPixelsPtr()[i]);
        pointer++;
    }
}

void Bitmap_na_Obrazek(sf::Image& image, volatile uint8_t* bitmap, uint32_t width, uint32_t height) {
    int setx = 0;
    int sety = 0;
    for (uint32_t i = 0; i < width * height * 3; i += 3) {
        image.setPixel(setx, sety, sf::Color(bitmap[i], bitmap[i + 1], bitmap[i + 2]));
        setx++;
        if (setx == width) {
            setx = 0;
            sety++;
        }
    }
}

int main() {
    dllHandleAsm = LoadLibrary(TEXT("JAAsm.dll"));
OdbicieASM flipASM = (OdbicieASM)GetProcAddress(dllHandleAsm, "OdbicieASM");
CzasASM Clockticks = (CzasASM)GetProcAddress(dllHandleAsm, "PomiarCzasu");

dllHandleCpp = LoadLibrary(TEXT("Odbicie.dll"));
Odbicie flipVertical = (Odbicie)GetProcAddress(dllHandleCpp, "flipVertical");

sf::RenderWindow window(sf::VideoMode(1900, 800), "Image Processing");
window.setFramerateLimit(60);

sf::Font font;
if (!font.loadFromFile("DejaVuSans.ttf")) {
    std::cerr << "Nie można zaladowac czcionki" << std::endl;
    return -1;
}

// Elementy UI
std::string filePath = "";
bool isFilePathActive = false;
bool processingStarted = false;

// Przyciski do wprowadzania ścieżki
sf::RectangleShape filePathButton(sf::Vector2f(400, 40));
filePathButton.setPosition(50, 50);
filePathButton.setFillColor(sf::Color::Blue);

sf::Text filePathButtonText("Nacisnij, aby wpisac sciezke do pliku", font, 20);
filePathButtonText.setPosition(55, 55);

sf::Text inputText(filePath, font, 20);
inputText.setPosition(55, 55);
inputText.setFillColor(sf::Color::White);

int threadCount = 1;
sf::Text threadText("Liczba watkow: " + std::to_string(threadCount), font, 20);
threadText.setPosition(50, 120);

sf::RectangleShape incrementThreadButton(sf::Vector2f(30, 30));
incrementThreadButton.setPosition(250, 120);
incrementThreadButton.setFillColor(sf::Color::Green);

sf::RectangleShape decrementThreadButton(sf::Vector2f(30, 30));
decrementThreadButton.setPosition(290, 120);
decrementThreadButton.setFillColor(sf::Color::Red);

sf::Text arrowUp(L"\u2191", font, 20); 
arrowUp.setFillColor(sf::Color::Black);
arrowUp.setPosition(255, 120);

sf::Text arrowDown(L"\u2193", font, 20); 
arrowDown.setFillColor(sf::Color::White);
arrowDown.setPosition(295, 120);

bool useCpp = true;
sf::Text modeText("Tryb: C++", font, 20);
modeText.setPosition(50, 180);

sf::RectangleShape toggleModeButton(sf::Vector2f(100, 30));
toggleModeButton.setPosition(160, 180);

toggleModeButton.setFillColor(sf::Color::Cyan);

sf::Text choiceText("Choice", font, 20);
choiceText.setFillColor(sf::Color::Black);
choiceText.setPosition(170, 185); 


sf::RectangleShape processButton(sf::Vector2f(200, 50));
processButton.setFillColor(sf::Color::Green);
processButton.setPosition(50, 240);

sf::Text processButtonText("Przetworz obraz", font, 20);
processButtonText.setPosition(60, 250);

sf::Text elapsedTimeText("", font, 20);
elapsedTimeText.setPosition(50, 310);  // Pozycja tekstu z czasem wykonania
double elapsedTime = 0.0;

sf::Texture texture;
sf::Sprite sprite;

bool showCursor = false; // Flaga do migającego kursora
sf::Clock cursorClock; // Zegar do odliczania migania kursora

while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            window.close();


        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            // Obsługa kliknięcia na przycisk wprowadzania ścieżki
            if (filePathButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                isFilePathActive = true;
                filePathButtonText.setString("");  // Usunięcie tekstu z przycisku
            }

            // Inkrementacja liczby wątków
            if (incrementThreadButton.getGlobalBounds().contains(mousePos.x, mousePos.y) && threadCount < 64) {
                threadCount++;
                threadText.setString("Liczba watkow: " + std::to_string(threadCount));
            }

            // Dekrementacja liczby wątków
            if (decrementThreadButton.getGlobalBounds().contains(mousePos.x, mousePos.y) && threadCount > 1) {
                threadCount--;
                threadText.setString("Liczba watkow: " + std::to_string(threadCount));
            }

            // Przełącznik C++/ASM
            if (toggleModeButton.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
                useCpp = !useCpp;
                modeText.setString(useCpp ? "Tryb: C++" : "Tryb: ASM");
            }

            // Obsługa kliknięcia na przycisk "Przetwórz obraz"
            if (processButton.getGlobalBounds().contains(mousePos.x, mousePos.y) && !processingStarted) {
                processingStarted = true;

                sf::Image image;
                if (!image.loadFromFile(filePath)) {
                    std::cout << "Nie można załadować obrazu" << std::endl;
                    processingStarted = false;
                    continue;
                }

                uint32_t width = image.getSize().x;
                uint32_t height = image.getSize().y;
                uint64_t size = width * height * 3;
                volatile uint8_t* bitmap = new uint8_t[size];

                Obrazek_na_Bitmap(image, bitmap);

                std::vector<std::thread> threads;
                uint32_t rowsPerThread = height / threadCount;
                uint32_t remainderRows = height % threadCount;

                auto startTime = std::chrono::high_resolution_clock::now();

                uint32_t currentRow = 0;
                for (int i = 0; i < threadCount; ++i) {
                    uint32_t startRow = currentRow;
                    uint32_t endRow = startRow + rowsPerThread + (i < remainderRows ? 1 : 0);
                    currentRow = endRow;
                    if (useCpp) {
                        threads.emplace_back(flipVertical, bitmap, width,  startRow, endRow);
                    }
                    else {
                        threads.emplace_back(flipASM, bitmap, width, height, startRow, endRow);
                    }
                }

                for (auto& th : threads) th.join();
                auto stopTime = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = stopTime - startTime;
                elapsedTime = elapsed.count();  // Zapisanie czasu wykonania
                elapsedTimeText.setString("Czas przetwarzania: " + std::to_string(elapsedTime) + " sekund");

                Bitmap_na_Obrazek(image, bitmap, width, height);

                // Wybór nazwy pliku w zależności od trybu
                std::string outputFileName = useCpp ? "C++_obraz.bmp" : "ASM_obraz.bmp";
                image.saveToFile(outputFileName); // Zapisanie obrazu
                        
                texture.loadFromImage(image);
                sprite.setTexture(texture,true); // true jest po to żeby wczytywał zawsze nowy rozmiar
                delete[] bitmap;
                processingStarted = false;
            }
        }

        // Obsługa wpisywania ścieżki do pliku
        if (event.type == sf::Event::TextEntered && isFilePathActive) {
            if (event.text.unicode == 8 && !filePath.empty()) {
                filePath.pop_back();
            }
            else if (event.text.unicode < 128 && event.text.unicode != 13) {
                filePath += static_cast<char>(event.text.unicode);
            }
            inputText.setString(filePath);
        }
    }

    // Miganie kursora
    if (isFilePathActive) {
        if (cursorClock.getElapsedTime().asSeconds() >= 0.5f) { // Zmiana stanu co 0.5 sekundy
            showCursor = !showCursor;
            cursorClock.restart();
        }
    }
    else {
        showCursor = false; // Wyłączenie kursora, gdy nie edytujemy
    }

    window.clear();
    window.draw(filePathButton);
    if (filePath.empty() && !isFilePathActive) {
        window.draw(filePathButtonText); // Pokazujemy tekst przycisku, gdy nie wprowadzamy ścieżki
    }
    else {
        // Dodanie migającego kursora do ścieżki
        std::string displayText = filePath;
        if (isFilePathActive && showCursor) {
            displayText += "_"; // Dodanie kursora
        }
        inputText.setString(displayText);
        window.draw(inputText); // Pokazujemy wpisywany tekst
    }
    window.draw(threadText);
    window.draw(incrementThreadButton);
    window.draw(decrementThreadButton);
    window.draw(arrowUp);    // Wyświetlenie strzałki w górę
    window.draw(arrowDown);  // Wyświetlenie strzałki w dół
    window.draw(modeText);
    window.draw(toggleModeButton);
    window.draw(choiceText);
    window.draw(processButton);
    window.draw(processButtonText);
    window.draw(elapsedTimeText);  // Wyświetlanie czasu wykonania
    if (texture.getSize().x > 0 && texture.getSize().y > 0) {
        window.draw(sprite);
    }

    
    // Automatyczne skalowanie obrazu
    if (texture.getSize().x > 0 && texture.getSize().y > 0) {
        sprite.setPosition(600, 0);

        float scaleX = static_cast<float>(window.getSize().x) / texture.getSize().x;
        float scaleY = static_cast<float>(window.getSize().y) / texture.getSize().y;
        float scale = std::min(scaleX, scaleY);

        sprite.setScale(scale, scale);
        window.draw(sprite);
    }


    

    window.display();
}

FreeLibrary(dllHandleCpp);
FreeLibrary(dllHandleAsm);

return 0;


}
