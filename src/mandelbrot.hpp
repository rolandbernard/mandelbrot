/*  mandelbrot.hpp
 * Name Mandelbrot-Modul
 * Modul zum berechnen der Mandelbrot-Menge mithilfe von OpenCL
 * Autor: Roland Bernard
 * Lizenz: (C) Copyright 2018 by Roland Bernard. All rights reserved.
 * */

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>

class mandelbrot
{
public:
    // Speichern einer Farbe
    struct color
    {
        unsigned char r;    // Rot
        unsigned char g;    // Grün
        unsigned char b;    // Blau
        unsigned char pad;  // Padding für 32-bit
    };
    // Speichern einer Auflösung
    struct res
    {
        size_t x;
        size_t y;
    };
    // Speichern einer Position
    struct pos
    {
        double x;
        double y;
    };
    // Spechern eines Bereiches
    struct rect
    {
        pos tl;
        pos br;
    };

private:
    cl_platform_id _platform_id;        // OpenCL Platform (Treiber)
    cl_device_id _device_id;            // OpenCL Device (GPU)
    cl_context _context;                // OpenCL Context
    cl_command_queue _command_queue;    // OpenCL Command Queue
    cl_program _program;                // OpenCL Programm (mandelbrot.cl)
    cl_kernel _kernel;                  // OpenCL Kernel (computeColors)
    cl_mem _image;                      // OpenCL Buffer zum speichern des Bildes

public:
    // Der Konstruktor initialisiert die OpenCL umgebung
    mandelbrot();
    // Der Destructor beendet alles sicher
    ~mandelbrot();

    // Listet alle verfügbaren Devices in allen Platformen auf, und gibt sie aus.
    void listDevices();

    /* Erstellt den Buffer für das Image in _image
     * @param res Die auflösung des Bildes
     * @param ptr Ein Zeiger zum Buffer im RAM
     */
    void createBuffer(mandelbrot::res res, void* ptr);

    // Löscht den Buffer _image
    void deleteBuffer();

    /* Berechnet die Abbildung der Mandelbrot-Menge und speichet das ergebnis in ret
     * @param ret Ein Zeiger zum Buffer im RAM
     * @param res Die Auflösung des Bildes
     * @param pos Die Fläche die berechnet werden soll
     * @param i Die maximale Anzahl an Iterationen
     * @param i Die Anzahl Samples pro Pixel
     */
    void computeImage(mandelbrot::color* ret, mandelbrot::res res, mandelbrot::rect pos, size_t i, size_t samples);
};
