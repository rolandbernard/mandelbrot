/*  main.c
 * Name: Mandelbrot-Frontend
 * Zeichnet die vom Mandelbrot-Modul generierten Bilder in ein Fenster und erlaubt bessere Navigation.
 * Autor: Roland Bernard
 * Lizenz: (C) Copyright 2018 by Roland Bernard. All rights reserved.
 */

#include <SDL2/SDL.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <chrono>

#include "mandelbrot.hpp"

// Konstanten
    // Default-Werte
#define DEF_ITERATIONEN 100
#define DEF_SAMPLES 1
#define DEF_X0 -2
#define DEF_Y0 2
#define DEF_X1 2
#define DEF_Y1 -2

    // Fenstergröße
#define RES_WIDTH 700
#define RES_HEIGHT 700

// Variablen zur Synkronsiation
std::condition_variable calculate;
std::mutex calcLock;
bool end;

// Variablen zum Zeichnen der Mandelbrot-Menge
mandelbrot::pos mouse;          // Momentane Mausposition
mandelbrot::pos mouseStart;     // Mausposition am anfang des Auswählens
SDL_Texture* tex;               // Textur die das Abbild der Mandelbrot-Menge enthält

// Variablen zum errechnen der Mandelbrot-Menge
mandelbrot::color* colorBuffer;     // Buffer zum abspeichern der Farben
mandelbrot::res res;                // Auflösung in der berechnet werden soll
mandelbrot::rect calcArea;          // Fläche die berechnet werden soll
size_t iterationen;                 // Maximale Anzahl an Iterationen der Berechnung
size_t samples;                     // Anzahl an Samples pro pixel
mandelbrot* brot;                   // Mandelbrot-Modul

// Threat zur Abarbeitung von Eingebe
void inputThread()
{
    bool stay = true;       // False falls das Programm geschlossen wurde
    bool button = false;    // True falls eine taste gedrücht ist, andernfals false
    bool stop = false;      // True falls die mausposition nichtmehr geändert werden soll
    mandelbrot::rect tmpArea;   // Variable zum Speichern der Ausgewählten Fläche

    SDL_Event event;        // Das momentan bearbeitete Event
    while(stay)
    {
        // Lesen des nächsten Events
        SDL_WaitEvent(&event);

        // Auswerten des erhaltenen Events
        switch(event.type)
        {
            case SDL_QUIT:
                // Programm soll schiesen
                stay = false;
                break;
            case SDL_KEYDOWN:
                // Taste wurde gedrückt
                switch(event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_ESCAPE:
                        // ESC == Programm soll schliesen
                        stay = false;
                        break;
                    case SDL_SCANCODE_UP:
                        // Erhöhen der maximalen Iterationen
                        iterationen += iterationen/10 + 1;
                        break;
                    case SDL_SCANCODE_DOWN:
                        // Erniedrigung der maximalen Iterationen
                        if(iterationen > 1)
                            iterationen -= iterationen/10 + 1;
                        break;
                    case SDL_SCANCODE_RIGHT:
                        // Erhöhung der Sample pro Pixel
                        samples++;
                        break;
                    case SDL_SCANCODE_LEFT:
                        // Erniedrigung der Sample pro Pixel
                        if(samples > 1)
                            samples--;
                        break;
                    case SDL_SCANCODE_RETURN:
                        // Die Ausgewählte Fläche wird zur zu berechnenden gemacht
                        calcArea = tmpArea;
                        // Die Maus soll eine neue Fläche auswählen können
                        stop = false;
                        mouse.x = 0;
                        mouse.y = 0;
                        mouseStart.x = 0;
                        mouseStart.y = 0;
                        // Benachrichtigen des calc-Threads das gerechnet werden muss
                        calculate.notify_all();
                        // Ausgabe nützlicher informationen
                        std::cout << "[" << (calcArea.tl.x + calcArea.br.x)/2 << "/" << (calcArea.tl.y + calcArea.br.y)/2
                                    << ":" << (calcArea.tl.x - calcArea.br.x) << "/" << (calcArea.tl.y - calcArea.br.y)
                                    << " i = " << iterationen << ", s = " << samples << "]\n";
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                        // Zurüchsetzen auf die Ausgangsposition
                        tmpArea.tl.x = DEF_X0;
                        tmpArea.tl.y = DEF_Y0;
                        tmpArea.br.x = DEF_X1;
                        tmpArea.br.y = DEF_Y1;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                // Mauskilck
                    // Die Startposition wird gesetzt
                mouseStart.x = event.button.x;
                mouseStart.y = event.button.y;
                    // Ein Mausknopf ist gedrückt
                button = true;
                    // Es soll eine neue Fläche ausgewählt werden können
                stop = false;
                break;
            case SDL_MOUSEBUTTONUP:
                // Der Mausknopf ist nichtmehr gedrückt
                button = false;
                // Die ausgewählte Fläche wird berechnet
                tmpArea.tl.x = calcArea.tl.x + (calcArea.br.x - calcArea.tl.x)*mouseStart.x/res.x;
                tmpArea.tl.y = calcArea.tl.y + (calcArea.br.y - calcArea.tl.y)*mouseStart.y/res.y;
                tmpArea.br.x = calcArea.tl.x + (calcArea.br.x - calcArea.tl.x)*mouse.x/res.x;
                tmpArea.br.y = calcArea.tl.y + (calcArea.br.y - calcArea.tl.y)*mouse.y/res.y;
                // Die Auswahl soll nicht bewegt werden
                stop = true;
                break;
            case SDL_MOUSEMOTION:
                // Mausbewegung
                // Falls sich die Auswahl ändern soll
                if(!stop)
                {
                    // Setzen der Mausposition
                    mouse.x = event.motion.x;
                    mouse.y = event.motion.y;
                    // Falls kein Mouseknopf gedrückt wurde
                    if(!button)
                    {
                        // Startposition == Mausposition
                        mouseStart.x = event.motion.x;
                        mouseStart.y = event.motion.y;
                    }
                }
                break;
            default:
                break;
        }
    }
}

// Thread zum errechnen der Darstellung mithilfe des Mandelbrot-Moduls
void calculationThread()
{
    // Solange nicht beendet werden soll
    while(!end)
    {
        // Berechnen des Bildes
        brot->computeImage(colorBuffer, res, calcArea, iterationen, samples);

        // Übertragen des Bildes in die Textur
        SDL_UpdateTexture(tex, NULL, (void*)colorBuffer, res.x * sizeof(mandelbrot::color));

        // Warten bis die Berechnung wieder fon nöten ist
        std::unique_lock<std::mutex> lck(calcLock);
        calculate.wait(lck);
    }
}

/* Thread zum Zeichne der Operfläche mithilfe von SDL
 * @param renderer Der zu nutzende Renderer
 */
void drawingThread(SDL_Renderer* renderer)
{
    // Rect zum temporären speichern eines Rechtecks -> Auswahl
    SDL_Rect rect;

    // Setzt den Blend-Mode des Renderers für transparente Auswahl
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Solange nicht beendet werden soll
    while(!end)
    {
        // Rendern der Textur
        SDL_RenderCopy(renderer, tex, NULL, NULL);
        // Einstellung des Rects für die Auswahl
        rect.x = mouseStart.x;
        rect.y = mouseStart.y;
        rect.w = mouse.x - mouseStart.x;
        rect.h = mouse.y - mouseStart.y;
        // Renderen der Auswahl
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 125);
        SDL_RenderFillRect(renderer, &rect);
        // Anzeigen des Framebuffers
        SDL_RenderPresent(renderer);
        // 50 ms Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main()
{
    // Any infringment of the given Copyright might result in legal actions
    std::cout << "(C) Copyright 2018 by Roland Bernard. All rights reserved.\n";
    end = false;
    std::cout.precision(16);

    // Setzen der Fenstergröse
    res.x = RES_WIDTH;
    res.y = RES_HEIGHT;

    // Setzen der Default-Werte
    iterationen = DEF_ITERATIONEN;
    samples = DEF_SAMPLES;
    calcArea.tl.x = DEF_X0;
    calcArea.tl.y = DEF_Y0;
    calcArea.br.x = DEF_X1;
    calcArea.br.y = DEF_Y1;

    // Initzialisieren von SDL
    SDL_Init(SDL_INIT_EVERYTHING);

    // Erstellen einer neuen Fensters
    SDL_Window* window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, res.x, res.y, 0);

    // Erstellen eines neuen Renderers für des eben erstelte Fenster
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    // Erstellen der Textur und des Buffers
    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, res.x, res.y);
    colorBuffer = new mandelbrot::color[res.x * res.y];

    // Initialisierung des Mandelbrot-Moduls
    brot = new mandelbrot();
    brot->listDevices();

    // Erstellen des OpenCL-Buffers mit der benötigten größe
    brot->createBuffer(res, (void*)colorBuffer);

    // Starten der drei Threats
    std::thread* input = new std::thread(inputThread);
    std::thread* calc = new std::thread(calculationThread);
    std::thread* draw = new std::thread(drawingThread, renderer);

    // Warten bis der Input Thread beendet ist => Das Programm soll schliesen
    input->join();

    // Das Programm soll schliesen
    end = true;

    // Entsperren des calc-Threads
    calculate.notify_all();

    // Beenden der beiden übrigen Threads
    calc->join();
    draw->join();

    // Löschen des OpenCL-Buffers
    brot->deleteBuffer();
    // Sicheres beenden des Mandelbrots
    delete brot;

    // Zerstören des Fensters
    SDL_DestroyWindow(window);
    // Beenden von SDL
    SDL_Quit();

    return 0;
}
