// =============================================================================
//  VOLE MACHINE SIMULATOR  -  SFML GUI  (Dark / Cyberpunk theme)
// =============================================================================
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "VoleSimulator.h"  

#if SFML_VERSION_MAJOR >= 3
#define VOLE_SFML3 1
#endif

namespace backend {

inline int  readPC(VoleSimulator& s)               { return static_cast<int>(s.getCPU().getPC()) & 0xFF; }
inline int  readIR(VoleSimulator& s)               { return static_cast<int>(s.getCPU().getIR()) & 0xFFFF; }
inline int  readReg(VoleSimulator& s, int i)       { return static_cast<int>(s.getCPU().getRegister(i)) & 0xFF; }
inline bool isHalted(VoleSimulator& s)           { return s.getCPU().isHalted(); }

// ADAPTED: CPU step requires passing the Memory object
inline void step(VoleSimulator& s)               { s.getCPU().step(s.getMemory()); }

// ADAPTED: Memory read/write matching standard Memory class methods (read/write or get/set)
inline int  readMem(VoleSimulator& s, int a)       { return static_cast<int>(s.getMemory().read(a)) & 0xFF; }
inline void writeMem(VoleSimulator& s, int a, int v) { s.getMemory().write(a, v); }

// Register write
inline void writeReg(VoleSimulator& s, int i, int v) { s.getCPU().setRegister(i, v); }

// Program loader via Memory class
inline bool loadProgram(VoleSimulator& s, const std::string& path) {
    try {
        s.getMemory().loadFromFile(path);
        s.getCPU().resetCPU(); 
        return true;
    } catch (...) {
        return false;
    }
}

}

// =============================================================================
//  Value formatting utilities
// =============================================================================
namespace util {

inline char hexDigit(int n) { return "0123456789ABCDEF"[n & 0xF]; }

inline std::string toBinary8(int v) {
    std::string s(8, '0');
    for (int i = 0; i < 8; ++i) if (v & (1 << i)) s[7 - i] = '1';
    return s;
}
inline std::string toBinary4(int v) {
    std::string s(4, '0');
    for (int i = 0; i < 4; ++i) if (v & (1 << i)) s[3 - i] = '1';
    return s;
}
inline std::string toHex2(int v) {
    char b[8]; std::snprintf(b, sizeof b, "%02X", v & 0xFF); return b;
}
inline std::string toHex4(int v) {
    char b[8]; std::snprintf(b, sizeof b, "%04X", v & 0xFFFF); return b;
}
inline int toSigned(int v) { v &= 0xFF; return v >= 128 ? v - 256 : v; }

inline double toVoleFloat(int v) {
    v &= 0xFF;
    int sign = (v >> 7) & 1, e = (v >> 4) & 7, m = v & 0xF;
    if (e == 0 && m == 0) return 0.0;
    double val = std::ldexp(m / 16.0, e - 4);
    return sign ? -val : val;
}
inline std::string floatToString(double d) {
    char b[32]; std::snprintf(b, sizeof b, "%.6g", d); return b;
}

struct Decoded { std::string mnemonic, text; };

inline Decoded decode(int ir) {
    int op = (ir >> 12) & 0xF, r = (ir >> 8) & 0xF, x = (ir >> 4) & 0xF, y = ir & 0xF, xy = ir & 0xFF;
    std::string R  = std::string("Register ") + hexDigit(r);
    std::string S  = std::string("Register ") + hexDigit(x);
    std::string T  = std::string("Register ") + hexDigit(y);
    std::string XY = toHex2(xy);
    if (ir == 0) return {"---", "No instruction yet (IR = 0000)."};
    switch (op) {
    case 0x1: return {"LOAD",  "LOAD " + R + " with the bit pattern found at memory address " + XY};
    case 0x2: return {"LOAD",  "LOAD " + R + " with immediate value " + XY};
    case 0x3:
        if (xy == 0) return {"OUT", "STORE the contents of " + R + " to the SCREEN (memory address 00)"};
        return {"STORE", "STORE the bit pattern of " + R + " into memory address " + XY};
    case 0x4:
        if (r == 0) return {"MOVE", "MOVE the bit pattern of " + S + " into " + T};
        return {"???", "Invalid MOVE."};
    case 0x5: return {"ADD",  "ADD " + S + " and " + T + " as two's complement integers; place result in " + R};
    case 0x6: return {"FADD", "ADD " + S + " and " + T + " as floating point values; place result in " + R};
    case 0x7: return {"OR",   "OR the bit patterns of " + S + " and " + T + "; place result in " + R};
    case 0x8: return {"AND",  "AND the bit patterns of " + S + " and " + T + "; place result in " + R};
    case 0x9: return {"XOR",  "XOR the bit patterns of " + S + " and " + T + "; place result in " + R};
    case 0xA: return {"ROR",  "ROTATE the bit pattern in " + R + " right " + std::to_string(y) + " time(s)"};
    case 0xB: return {"JUMP", "JUMP to address " + XY + " if " + R + " equals Register 0"};
    case 0xC: return {"HALT", "HALT execution."};
    default:  return {"???",  "Unknown instruction."};
    }
}

}

// =============================================================================
//  SFML 2 / 3 compatibility helpers
// =============================================================================
static sf::FloatRect makeFR(float l, float t, float w, float h) {
#ifdef VOLE_SFML3
    return sf::FloatRect({l, t}, {w, h});
#else
    return sf::FloatRect(l, t, w, h);
#endif
}
static sf::VideoMode makeVideoMode(unsigned w, unsigned h) {
#ifdef VOLE_SFML3
    return sf::VideoMode({w, h});
#else
    return sf::VideoMode(w, h);
#endif
}

struct Input {
    enum Type { None, Closed, Resized, Move, Press, Release, Wheel, Text } type = None;
    float x = 0, y = 0, delta = 0;
    unsigned w = 0, h = 0, unicode = 0;
    bool left = false;
};

static bool pollInput(sf::RenderWindow& win, Input& in) {
    in = Input{};
#ifdef VOLE_SFML3
    std::optional<sf::Event> e = win.pollEvent();
    if (!e) return false;
    if (e->is<sf::Event::Closed>()) in.type = Input::Closed;
    else if (const auto* r = e->getIf<sf::Event::Resized>()) {
        in.type = Input::Resized; in.w = r->size.x; in.h = r->size.y;
    } else if (const auto* m = e->getIf<sf::Event::MouseMoved>()) {
        in.type = Input::Move; in.x = float(m->position.x); in.y = float(m->position.y);
    } else if (const auto* p = e->getIf<sf::Event::MouseButtonPressed>()) {
        in.type = Input::Press; in.left = (p->button == sf::Mouse::Button::Left);
        in.x = float(p->position.x); in.y = float(p->position.y);
    } else if (const auto* q = e->getIf<sf::Event::MouseButtonReleased>()) {
        in.type = Input::Release; in.left = (q->button == sf::Mouse::Button::Left);
        in.x = float(q->position.x); in.y = float(q->position.y);
    } else if (const auto* w = e->getIf<sf::Event::MouseWheelScrolled>()) {
        in.type = Input::Wheel; in.delta = w->delta;
        in.x = float(w->position.x); in.y = float(w->position.y);
    } else if (const auto* t = e->getIf<sf::Event::TextEntered>()) {
        in.type = Input::Text; in.unicode = t->unicode;
    }
    return true;
#else
    sf::Event e;
    if (!win.pollEvent(e)) return false;
    switch (e.type) {
    case sf::Event::Closed: in.type = Input::Closed; break;
    case sf::Event::Resized: in.type = Input::Resized; in.w = e.size.width; in.h = e.size.height; break;
    case sf::Event::MouseMoved: in.type = Input::Move; in.x = float(e.mouseMove.x); in.y = float(e.mouseMove.y); break;
    case sf::Event::MouseButtonPressed:
        in.type = Input::Press; in.left = (e.mouseButton.button == sf::Mouse::Left);
        in.x = float(e.mouseButton.x); in.y = float(e.mouseButton.y); break;
    case sf::Event::MouseButtonReleased:
        in.type = Input::Release; in.left = (e.mouseButton.button == sf::Mouse::Left);
        in.x = float(e.mouseButton.x); in.y = float(e.mouseButton.y); break;
    case sf::Event::MouseWheelScrolled:
        in.type = Input::Wheel; in.delta = e.mouseWheelScroll.delta;
        in.x = float(e.mouseWheelScroll.x); in.y = float(e.mouseWheelScroll.y); break;
    case sf::Event::TextEntered: in.type = Input::Text; in.unicode = e.text.unicode; break;
    default: break;
    }
    return true;
#endif
}

namespace theme {
const sf::Color BG(10, 12, 18), PANEL(18, 22, 30), HEADER(26, 32, 44), FIELD(13, 17, 24);
const sf::Color BORDER(0, 120, 115), GREEN(57, 255, 20), CYAN(0, 229, 255);
const sf::Color MAG(255, 64, 200), YEL(255, 214, 0), TXT(220, 230, 235), DIM(120, 135, 145);
const sf::Color ROW_A(18, 22, 30), ROW_B(23, 28, 38), ROW_HOVER(36, 50, 66);
}

static float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }

struct Box {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

class Gfx {
public:
    explicit Gfx(sf::RenderWindow& w) : win(w)
#ifdef VOLE_SFML3
    , text(font)
#endif
    {}

    bool loadFont() {
        const char* paths[] = {"font.ttf", "assets/font.ttf", "C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/cour.ttf"};
        for (const char* p : paths) {
#ifdef VOLE_SFML3
            if (font.openFromFile(p)) return true;
#else
            if (font.loadFromFile(p)) { text.setFont(font); return true; }
#endif
        }
        return false;
    }

    void rect(const Box& b, sf::Color fill, sf::Color outline = sf::Color::Transparent, float th = 0) {
        shape.setSize(sf::Vector2f(b.w, b.h));
        shape.setPosition(sf::Vector2f(b.x, b.y));
        shape.setFillColor(fill);
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(-th);
        win.draw(shape);
    }
    void txt(const std::string& s, float x, float y, unsigned size, sf::Color c) {
        text.setString(s); text.setCharacterSize(size); text.setFillColor(c);
        text.setPosition(sf::Vector2f(std::floor(x), std::floor(y)));
        win.draw(text);
    }
    float textW(const std::string& s, unsigned size) {
        text.setString(s); text.setCharacterSize(size);
        auto b = text.getLocalBounds();
#ifdef VOLE_SFML3
        return b.position.x + b.size.x;
#else
        return b.left + b.width;
#endif
    }
    float lineH(unsigned size) { return font.getLineSpacing(size); }

    void txtV(const std::string& s, const Box& b, float xoff, unsigned size, sf::Color c) {
        txt(s, b.x + xoff, b.y + (b.h - lineH(size)) * 0.5f, size, c);
    }
    void txtC(const std::string& s, const Box& b, unsigned size, sf::Color c) {
        txt(s, b.x + (b.w - textW(s, size)) * 0.5f, b.y + (b.h - lineH(size)) * 0.5f, size, c);
    }
    void txtR(const std::string& s, const Box& b, float xoff, unsigned size, sf::Color c) {
        txt(s, b.x + b.w - xoff - textW(s, size), b.y + (b.h - lineH(size)) * 0.5f, size, c);
    }

    void setClip(const Box& b, float W, float H) {
        sf::View v(makeFR(b.x, b.y, b.w, b.h));
        v.setViewport(makeFR(b.x / W, b.y / H, b.w / W, b.h / H));
        win.setView(v);
    }
    void clearClip(float W, float H) { win.setView(sf::View(makeFR(0, 0, W, H))); }

    sf::RenderWindow& win;
    sf::Font font;
    sf::Text text;
    sf::RectangleShape shape;
};

struct Layout {
    float W = 0, H = 0, rowH = 24;
    unsigned fs = 14;
    Box header;
    Box memPanel, memTitle, memCols, memList, memTrack;
    Box regPanel, regTitle, regCols, regList, regInfo;
    Box pcBox, irBox, decBox, fileBox, screenBox;
    Box btn[6];
};

static Layout computeLayout(float W, float H) {
    Layout L; L.W = W; L.H = H;
    L.fs = (unsigned)clampf(std::floor(W / 105.f), 11.f, 17.f);
    L.rowH = std::floor(L.fs * 1.75f);

    const float m = 10, headerH = 46, titleH = 32, g = 8;
    L.header = {0, 0, W, headerH};
    float y0 = headerH + m, ph = H - y0 - m;
    float total = W - 4 * m;
    float wl = total * 0.33f, wc = total * 0.29f, wr = total - wl - wc;
    float xl = m, xc = xl + wl + m, xr = xc + wc + m;
    float colH = L.rowH + 2;

    L.memPanel = {xl, y0, wl, ph};
    L.memTitle = {xl, y0, wl, titleH};
    L.memCols  = {xl, y0 + titleH, wl, colH};
    L.memList  = {xl, y0 + titleH + colH, wl, ph - titleH - colH};
    L.memTrack = {xl + wl - 13, L.memList.y + 2, 10, L.memList.h - 4};

    L.regPanel = {xc, y0, wc, ph};
    L.regTitle = {xc, y0, wc, titleH};
    L.regCols  = {xc, y0 + titleH, wc, colH};
    L.regList  = {xc, y0 + titleH + colH, wc, 16 * L.rowH};
    float iy = L.regList.y + L.regList.h + m;
    L.regInfo  = {xc + 8, iy, wc - 16, y0 + ph - iy - 8};

    float y = y0;
    float pcH = clampf(ph * 0.085f, 48, 70);
    L.pcBox = {xr, y, wr, pcH}; y += pcH + g;
    float irH = clampf(ph * 0.26f, 130, 230);
    L.irBox = {xr, y, wr, irH}; y += irH + g;
    float decH = clampf(ph * 0.15f, 70, 130);
    L.decBox = {xr, y, wr, decH}; y += decH + g;
    float btnH = clampf(ph * 0.058f, 30, 44);
    L.fileBox = {xr, y, wr, btnH}; y += btnH + g;
    float bw = (wr - g) / 2;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 2; ++c) L.btn[r * 2 + c] = {xr + c * (bw + g), y, bw, btnH};
        y += btnH + g;
    }
    L.screenBox = {xr, y, wr, std::max(60.f, y0 + ph - y)};
    return L;
}

class App {
public:
    explicit App(VoleSimulator& s)
        : sim(s), win(makeVideoMode(1600, 900), "Vole Machine Simulator"), g(win) {
        win.setFramerateLimit(60);
        for (int i = 0; i < 16; ++i) regPrev[i] = backend::readReg(sim, i);
    }

    bool init() { return g.loadFont(); }

    void run() {
        L = computeLayout(float(win.getSize().x), float(win.getSize().y));
        while (win.isOpen()) {
            L = computeLayout(float(win.getSize().x), float(win.getSize().y));
            Input in;
            while (pollInput(win, in)) handleInput(in);
            L = computeLayout(float(win.getSize().x), float(win.getSize().y));
            update();
            draw();
        }
    }

private:
    VoleSimulator& sim;
    sf::RenderWindow win;
    Gfx g;
    Layout L;
    sf::Clock clock;

    float scroll = 0;
    bool dragging = false, mouseDown = false, running = false, editingFile = false;
    int lastPC = -1;
    unsigned long cycles = 0, totalSteps = 0;
    int regPrev[16];
    std::string screen, status = "Ready. Load a program to begin.", filename = "program1.txt";
    float mx = 0, my = 0;

    static constexpr unsigned long MAX_STEPS = 1000000;
    const float COLW[5] = {0.15f, 0.27f, 0.12f, 0.15f, 0.31f};

    float maxScroll() const { return std::max(0.f, 256 * L.rowH - L.memList.h); }
    void clampScroll() { scroll = clampf(scroll, 0, maxScroll()); }

    void ensureVisible(int addr) {
        float top = addr * L.rowH, bottom = top + L.rowH * (addr < 255 ? 2 : 1);
        if (top < scroll) scroll = top;
        else if (bottom > scroll + L.memList.h) scroll = bottom - L.memList.h;
        clampScroll();
    }
    void thumb(float& ty, float& th) const {
        float content = 256 * L.rowH, view = L.memList.h;
        th = std::max(28.f, L.memTrack.h * view / content);
        float ms = std::max(1.f, maxScroll());
        ty = L.memTrack.y + (L.memTrack.h - th) * (scroll / ms);
    }
    void dragTo(float y) {
        float ty, th; thumb(ty, th);
        float t = (y - L.memTrack.y - th * 0.5f) / std::max(1.f, L.memTrack.h - th);
        scroll = clampf(t, 0, 1) * maxScroll();
    }

    void syncRegSnapshot() { for (int i = 0; i < 16; ++i) regPrev[i] = backend::readReg(sim, i); }

    void appendScreen(int v) {
        v &= 0xFF;
        if (v == '\n' || v == '\r') screen += '\n';
        else if (v >= 32 && v < 127) screen += char(v);
        else { char b[8]; std::snprintf(b, sizeof b, "[%02X]", v); screen += b; }
        if (screen.size() > 8000) screen.erase(0, screen.size() - 8000);
    }

    bool doStep() {
        if (backend::isHalted(sim)) {
            running = false; status = "CPU is halted."; return false;
        }
        syncRegSnapshot();
        backend::step(sim);
        ++cycles; ++totalSteps;
        int ir = backend::readIR(sim);
        if (((ir >> 12) & 0xF) == 3 && (ir & 0xFF) == 0) appendScreen(backend::readReg(sim, (ir >> 8) & 0xF));
        if (backend::isHalted(sim) || ir == 0xC000) {
            running = false;
            status = "HALT reached after " + std::to_string(cycles) + " cycle(s).";
        } else {
            status = "Executed: " + util::decode(ir).mnemonic;
        }
        return true;
    }

    void onButton(int i) {
        switch (i) {
        case 0: {
            bool ok = backend::loadProgram(sim, filename);
            cycles = 0; totalSteps = 0; running = false; lastPC = -1;
            syncRegSnapshot();
            status = ok ? "Loaded '" + filename + "' successfully." : "Error loading '" + filename + "'.";
            break;
        }
        case 1: doStep(); break;
        case 2:
            if (running) { running = false; status = "Stopped."; }
            else if (backend::isHalted(sim)) status = "Already halted.";
            else { running = true; totalSteps = 0; status = "Running..."; }
            break;
        case 3:
            for (int r = 0; r < 16; ++r) backend::writeReg(sim, r, 0);
            syncRegSnapshot(); status = "Registers cleared."; break;
        case 4:
            for (int a = 0; a < 256; ++a) backend::writeMem(sim, a, 0);
            status = "Memory cleared."; break;
        case 5: screen.clear(); status = "Screen cleared."; break;
        }
    }

    bool buttonEnabled(int i) const { return !running || i == 2; }
    std::string buttonLabel(int i) const {
        static const char* names[6] = {"Load File", "Run One Cycle", "Run Until Halt",
                                       "Clear Registers", "Clear Memory", "Clear Screen"};
        if (i == 2 && running) return "Stop";
        return names[i];
    }
    sf::Color buttonAccent(int i) const {
        if (i == 2) return running ? theme::YEL : theme::CYAN;
        if (i >= 3) return theme::MAG;
        return theme::GREEN;
    }

    void handleInput(const Input& in) {
        switch (in.type) {
        case Input::Closed: win.close(); break;
        case Input::Resized: {
            const unsigned minW = 1100, minH = 680;
            if (in.w < minW || in.h < minH) {
                win.setSize(sf::Vector2u(std::max(in.w, minW), std::max(in.h, minH)));
                break;
            }
            g.clearClip(float(in.w), float(in.h));
            L = computeLayout(float(in.w), float(in.h));
            clampScroll();
            break;
        }
        case Input::Move: mx = in.x; my = in.y; if (dragging) dragTo(in.y); break;
        case Input::Wheel:
            if (L.memList.contains(in.x, in.y)) { scroll -= in.delta * L.rowH * 3; clampScroll(); }
            break;
        case Input::Release: if (in.left) { dragging = false; mouseDown = false; } break;
        case Input::Press:
            if (!in.left) break;
            mouseDown = true;
            if (L.memTrack.contains(in.x, in.y) && maxScroll() > 0) { dragging = true; dragTo(in.y); break; }
            editingFile = L.fileBox.contains(in.x, in.y) && !running;
            for (int i = 0; i < 6; ++i)
                if (L.btn[i].contains(in.x, in.y) && buttonEnabled(i)) { onButton(i); break; }
            break;
        case Input::Text:
            if (!editingFile) break;
            if (in.unicode == 8) { if (!filename.empty()) filename.pop_back(); }
            else if (in.unicode == 13 || in.unicode == 27) editingFile = false;
            else if (in.unicode >= 32 && in.unicode < 127 && filename.size() < 80) filename += char(in.unicode);
            break;
        default: break;
        }
    }

    void update() {
        sf::Vector2i mp = sf::Mouse::getPosition(win);
        mx = float(mp.x); my = float(mp.y);
        if (running) {
            for (int i = 0; i < 400 && running; ++i) {
                if (!doStep()) break;
                if (totalSteps >= MAX_STEPS) { running = false; status = "Limit reached."; }
            }
        }
        int pc = backend::readPC(sim);
        if (pc != lastPC) { lastPC = pc; ensureVisible(pc); }
        clampScroll();
    }

    void drawPanelFrame(const Box& b, const Box& title, const std::string& t, sf::Color accent) {
        g.rect(b, theme::PANEL, theme::BORDER, 1);
        g.rect(title, theme::HEADER);
        g.rect({title.x, title.y, 3, title.h}, accent);
        g.txtV(t, title, 14, L.fs + 1, accent);
    }

    void drawColumnHeader(const Box& b) {
        g.rect(b, theme::HEADER);
        g.rect({b.x, b.y + b.h - 1, b.w, 1}, theme::BORDER);
        static const char* names[5] = {"Address", "Binary", "Hex", "Int", "Floating"};
        float inner = b.w - 20, cx = b.x + 8;
        for (int c = 0; c < 5; ++c) {
            g.txtV(names[c], {cx, b.y, inner * COLW[c], b.h}, 0, L.fs - 1, theme::CYAN);
            cx += inner * COLW[c];
        }
    }

    void drawValueRow(const Box& row, const std::string& addr, int v, int idx, int hl, bool hover, float innerW) {
        sf::Color fill = (idx & 1) ? theme::ROW_B : theme::ROW_A;
        if (hover) fill = theme::ROW_HOVER;
        if (hl == 1) fill = hover ? sf::Color(130, 255, 95) : theme::GREEN;
        g.rect(row, fill);
        if (hl == 2) { g.rect(row, sf::Color(57, 255, 20, 40)); g.rect({row.x, row.y, 3, row.h}, theme::GREEN); }
        if (hl == 3) { g.rect(row, sf::Color(0, 229, 255, 38)); g.rect({row.x, row.y, 3, row.h}, theme::CYAN); }

        sf::Color dark(8, 12, 8);
        std::string cols[5] = {addr, util::toBinary8(v), util::toHex2(v), std::to_string(util::toSigned(v)),
                               util::floatToString(util::toVoleFloat(v))};
        sf::Color cc[5] = {theme::CYAN, sf::Color(160, 185, 195), theme::GREEN, theme::TXT, sf::Color(255, 120, 215)};
        float cx = row.x + 8;
        for (int c = 0; c < 5; ++c) {
            g.txtV(cols[c], {cx, row.y, innerW * COLW[c], row.h}, 0, L.fs, hl == 1 ? dark : cc[c]);
            cx += innerW * COLW[c];
        }
    }

    void drawMemory() {
        drawPanelFrame(L.memPanel, L.memTitle, "MAIN MEMORY  (256 bytes)", theme::GREEN);
        drawColumnHeader(L.memCols);
        int pc = backend::readPC(sim);
        float rowW = L.memList.w - 18, innerW = rowW - 20;

        g.setClip(L.memList, L.W, L.H);
        int first = std::max(0, int(scroll / L.rowH));
        int last = std::min(255, int((scroll + L.memList.h) / L.rowH));
        bool listHover = L.memList.contains(mx, my);
        for (int a = first; a <= last; ++a) {
            Box row{L.memList.x, L.memList.y + a * L.rowH - scroll, rowW, L.rowH};
            int hl = (a == pc) ? 1 : (a == ((pc + 1) & 0xFF) ? 2 : 0);
            bool hov = listHover && row.contains(mx, my) && mx < L.memTrack.x;
            drawValueRow(row, util::toHex2(a), backend::readMem(sim, a), a, hl, hov, innerW);
        }
        g.clearClip(L.W, L.H);

        g.rect(L.memTrack, theme::FIELD, theme::BORDER, 1);
        float ty, th; thumb(ty, th);
        Box tb{L.memTrack.x, ty, L.memTrack.w, th};
        bool hv = tb.contains(mx, my) || dragging;
        g.rect(tb, hv ? theme::CYAN : sf::Color(0, 150, 170));
    }

    void drawRegisters() {
        drawPanelFrame(L.regPanel, L.regTitle, "REGISTERS  (R0 - RF)", theme::CYAN);
        drawColumnHeader(L.regCols);
        float innerW = L.regList.w - 20;
        for (int i = 0; i < 16; ++i) {
            Box row{L.regList.x, L.regList.y + i * L.rowH, L.regList.w, L.rowH};
            int v = backend::readReg(sim, i);
            int hl = (cycles > 0 && regPrev[i] != v) ? 3 : 0;
            char name[4]; std::snprintf(name, sizeof name, "R%c", util::hexDigit(i));
            drawValueRow(row, name, v, i, hl, row.contains(mx, my), innerW);
        }

        const Box& b = L.regInfo;
        if (b.h < 60) return;
        g.rect(b, theme::FIELD, theme::BORDER, 1);
        unsigned fs = L.fs;
        float y = b.y + 8, lh = g.lineH(fs) + 3;
        g.txt("MACHINE STATUS", b.x + 10, y, fs, theme::CYAN); y += lh + 2;
        bool halted = backend::isHalted(sim);
        std::string st = running ? "RUNNING" : (halted ? "HALTED" : "READY");
        sf::Color sc = running ? theme::YEL : (halted ? theme::MAG : theme::GREEN);
        g.txt("State   : ", b.x + 10, y, fs, theme::DIM); g.txt(st, b.x + 10 + g.textW("State   : ", fs), y, fs, sc); y += lh;
        if (y + lh > b.y + b.h) return;
        g.txt("Cycles  : " + std::to_string(cycles), b.x + 10, y, fs, theme::TXT); y += lh;
        if (y + lh > b.y + b.h) return;
        g.txt("Last op : " + util::decode(backend::readIR(sim)).mnemonic, b.x + 10, y, fs, theme::TXT); y += lh;
        if (y + lh > b.y + b.h) return;
        g.txt("Float   : S|EEE|MMMM  (excess-4)", b.x + 10, y, fs - 2, theme::DIM);
    }

    void drawPC() {
        const Box& b = L.pcBox;
        g.rect(b, theme::PANEL, theme::BORDER, 1);
        g.rect({b.x, b.y, 3, b.h}, theme::GREEN);
        g.txtV("PROGRAM COUNTER", b, 14, L.fs, theme::DIM);
        int pc = backend::readPC(sim);
        g.txtR("0x" + util::toHex2(pc), b, 14, unsigned(L.fs * 2), theme::GREEN);
    }

    void drawIR() {
        const Box& b = L.irBox;
        g.rect(b, theme::PANEL, theme::BORDER, 1);
        g.rect({b.x, b.y, 3, b.h}, theme::CYAN);
        g.txt("INSTRUCTION REGISTER (IR)", b.x + 14, b.y + 6, L.fs, theme::CYAN);

        int ir = backend::readIR(sim);
        unsigned big = unsigned(L.fs * 2);
        Box hb{b.x, b.y + L.fs + 10, b.w, big * 1.4f};
        std::string hx = util::toHex4(ir);
        g.txtC("0x" + hx, hb, big, theme::GREEN);

        float fy = hb.y + hb.h + 4, fh = b.y + b.h - fy - 8, gap = 8;
        float fw = (b.w - 16 - 3 * gap) / 4;
        const char* labels[4] = {"OP Code", "R", "X/S", "Y/T"};
        sf::Color cols[4] = {theme::CYAN, theme::GREEN, theme::YEL, theme::MAG};
        for (int i = 0; i < 4; ++i) {
            int nib = (ir >> (12 - 4 * i)) & 0xF;
            Box fb{b.x + 8 + i * (fw + gap), fy, fw, fh};
            bool hov = fb.contains(mx, my);
            g.rect(fb, hov ? sf::Color(24, 32, 44) : theme::FIELD, hov ? cols[i] : sf::Color(cols[i].r, cols[i].g, cols[i].b, 110), 1);
            g.txtC(labels[i], {fb.x, fb.y + 2, fb.w, float(L.fs) + 2}, L.fs - 2, theme::DIM);
            g.txtC(std::string(1, util::hexDigit(nib)), {fb.x, fb.y + L.fs, fb.w, fb.h - 2 * L.fs - 2}, unsigned(L.fs * 1.7f), cols[i]);
            g.txtC(util::toBinary4(nib), {fb.x, fb.y + fb.h - L.fs - 4, fb.w, float(L.fs) + 2}, L.fs - 2, sf::Color(150, 165, 175));
        }
    }

    std::vector<std::string> wrap(const std::string& s, float maxW, unsigned size) {
        std::vector<std::string> lines; std::string cur, word;
        auto flush = [&]() {
            if (word.empty()) return;
            std::string t = cur.empty() ? word : cur + " " + word;
            if (!cur.empty() && g.textW(t, size) > maxW) { lines.push_back(cur); cur = word; }
            else cur = t;
            word.clear();
        };
        for (char c : s) { if (c == ' ') flush(); else word += c; }
        flush();
        if (!cur.empty()) lines.push_back(cur);
        return lines;
    }

    void drawDecode() {
        const Box& b = L.decBox;
        g.rect(b, theme::PANEL, theme::BORDER, 1);
        g.rect({b.x, b.y, 3, b.h}, theme::YEL);
        util::Decoded d = util::decode(backend::readIR(sim));
        g.txt("DECODING MESSAGE", b.x + 14, b.y + 6, L.fs, theme::YEL);
        g.txtR(d.mnemonic, {b.x, b.y + 2, b.w, float(L.fs) + 10}, 14, L.fs + 2, theme::GREEN);
        float y = b.y + L.fs + 16, lh = g.lineH(L.fs) + 2;
        for (const auto& line : wrap(d.text, b.w - 28, L.fs)) {
            if (y + lh > b.y + b.h) break;
            g.txt(line, b.x + 14, y, L.fs, theme::TXT); y += lh;
        }
    }

    void drawFileBox() {
        const Box& b = L.fileBox;
        bool hov = b.contains(mx, my);
        g.rect(b, theme::FIELD, editingFile ? theme::CYAN : (hov ? theme::GREEN : theme::BORDER), editingFile ? 2 : 1);
        g.txtV("FILE:", b, 12, L.fs, theme::DIM);
        float off = 12 + g.textW("FILE: ", L.fs);
        std::string shown = filename;
        bool caret = editingFile && (int(clock.getElapsedTime().asSeconds() * 2) % 2 == 0);
        if (caret) shown += "_";
        while (shown.size() > 1 && off + g.textW(shown, L.fs) > b.w - 10) shown.erase(0, 1);
        g.txtV(shown, b, off, L.fs, theme::TXT);
    }

    void drawButtons() {
        for (int i = 0; i < 6; ++i) {
            const Box& b = L.btn[i];
            bool en = buttonEnabled(i), hov = en && b.contains(mx, my), down = hov && mouseDown;
            sf::Color ac = buttonAccent(i);
            sf::Color fill = theme::HEADER, outline(ac.r, ac.g, ac.b, 120), tc = ac;
            if (!en) { fill = sf::Color(20, 24, 30); outline = sf::Color(50, 58, 66); tc = sf::Color(80, 90, 100); }
            else if (down) { fill = ac; outline = ac; tc = sf::Color(8, 12, 8); }
            else if (hov) { fill = sf::Color(ac.r / 6 + 14, ac.g / 6 + 14, ac.b / 6 + 14); outline = ac; }
            g.rect(b, fill, outline, hov ? 2.f : 1.f);
            g.txtC(buttonLabel(i), b, L.fs + 1, tc);
        }
    }

    void drawScreen() {
        const Box& b = L.screenBox;
        g.rect(b, sf::Color(6, 14, 8), theme::GREEN, 1);
        g.rect({b.x, b.y, b.w, float(L.fs) + 14}, sf::Color(14, 30, 16));
        g.txt("SCREEN / OUTPUT", b.x + 14, b.y + 6, L.fs, theme::GREEN);

        float cw = std::max(1.f, g.textW("M", L.fs));
        size_t cols = size_t(std::max(1.f, std::floor((b.w - 24) / cw)));
        std::vector<std::string> lines(1);
        for (char c : screen) {
            if (c == '\n') lines.emplace_back();
            else { if (lines.back().size() >= cols) lines.emplace_back(); lines.back() += c; }
        }
        bool caret = int(clock.getElapsedTime().asSeconds() * 2) % 2 == 0;
        if (caret) lines.back() += "_";
        float lh = g.lineH(L.fs) + 2, top = b.y + L.fs + 20;
        size_t fit = size_t(std::max(1.f, std::floor((b.y + b.h - top - 6) / lh)));
        size_t start = lines.size() > fit ? lines.size() - fit : 0;
        float y = top;
        for (size_t i = start; i < lines.size(); ++i) { g.txt(lines[i], b.x + 12, y, L.fs, theme::GREEN); y += lh; }
    }

    void drawHeader() {
        g.rect(L.header, sf::Color(14, 18, 26));
        g.rect({0, L.header.h - 2, L.W, 2}, theme::CYAN);
        g.txtV("Vole Machine Simulator", L.header, 16, L.fs + 6, theme::CYAN);
        g.txtR(status, L.header, 16, L.fs, running ? theme::YEL : theme::GREEN);
    }

    void draw() {
        win.clear(theme::BG);
        drawHeader();
        drawMemory();
        drawRegisters();
        drawPC();
        drawIR();
        drawDecode();
        drawFileBox();
        drawButtons();
        drawScreen();
        win.display();
    }
};

int main() {
    VoleSimulator simulator;
    App app(simulator);
    if (!app.init()) {
        std::cerr << "ERROR: no usable font found. Place 'font.ttf' next to the executable.\n";
        return 1;
    }
    app.run();
    return 0;
}