#define GLFW_INCLUDE_GLCOREARB 1
#include <GLFW/glfw3.h>
#include <NanoCanvas.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

#include "core/ui/base.hpp"
#include "core/ui/mainui.hpp"
#include "core/globals.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>
#include <algorithm>

namespace top1::ui {

  static Key keyboardKey(int xKey, int mods) {
    switch (xKey) {

      // Rotaries
    case GLFW_KEY_Q:
      if (mods & GLFW_MOD_CONTROL) return K_BLUE_CLICK;
      return K_BLUE_UP;
    case GLFW_KEY_A:
      if (mods & GLFW_MOD_CONTROL) return K_BLUE_CLICK;
      return K_BLUE_DOWN;
    case GLFW_KEY_W:
      if (mods & GLFW_MOD_CONTROL) return K_GREEN_CLICK;
      return K_GREEN_UP;
    case GLFW_KEY_S:
      if (mods & GLFW_MOD_CONTROL) return K_GREEN_CLICK;
      return K_GREEN_DOWN;
    case GLFW_KEY_E:
      if (mods & GLFW_MOD_CONTROL) return K_WHITE_CLICK;
      return K_WHITE_UP;
    case GLFW_KEY_D:
      if (mods & GLFW_MOD_CONTROL) return K_WHITE_CLICK;
      return K_WHITE_DOWN;
    case GLFW_KEY_R:
      if (mods & GLFW_MOD_CONTROL) return K_RED_CLICK;
      return K_RED_UP;
    case GLFW_KEY_F:
      if (mods & GLFW_MOD_CONTROL) return K_RED_CLICK;
      return K_RED_DOWN;

    case GLFW_KEY_LEFT:  return K_LEFT;
    case GLFW_KEY_RIGHT: return K_RIGHT;

      // Tapedeck
    case GLFW_KEY_SPACE: return K_PLAY;
    case GLFW_KEY_Z:     return K_REC;
    case GLFW_KEY_F1:    return K_TRACK_1;
    case GLFW_KEY_F2:    return K_TRACK_2;
    case GLFW_KEY_F3:    return K_TRACK_3;
    case GLFW_KEY_F4:    return K_TRACK_4;

      // Numbers
    case GLFW_KEY_1:     return K_1;
    case GLFW_KEY_2:     return K_2;
    case GLFW_KEY_3:     return K_3;
    case GLFW_KEY_4:     return K_4;
    case GLFW_KEY_5:     return K_5;
    case GLFW_KEY_6:     return K_6;
    case GLFW_KEY_7:     return K_7;
    case GLFW_KEY_8:     return K_8;
    case GLFW_KEY_9:     return K_9;
    case GLFW_KEY_0:     return K_0;

    case GLFW_KEY_T:     if (mods & GLFW_MOD_CONTROL) return K_TAPE; else break;
    case GLFW_KEY_Y:     if (mods & GLFW_MOD_CONTROL) return K_MIXER; else break;
    case GLFW_KEY_U:     if (mods & GLFW_MOD_CONTROL) return K_SYNTH; else break;
    case GLFW_KEY_G:     if (mods & GLFW_MOD_CONTROL) return K_METRONOME; else break;
    case GLFW_KEY_H:     if (mods & GLFW_MOD_CONTROL) return K_SAMPLER; else break;
    case GLFW_KEY_J:     if (mods & GLFW_MOD_CONTROL) return K_DRUMS; else break;

    case GLFW_KEY_L:     return K_LOOP;
    case GLFW_KEY_I:     return K_LOOP_IN;
    case GLFW_KEY_O:     return K_LOOP_OUT;

    case GLFW_KEY_X:     return K_CUT;
    case GLFW_KEY_C:     if (mods & GLFW_MOD_CONTROL) return K_LIFT; else break;
    case GLFW_KEY_V:     if (mods & GLFW_MOD_CONTROL) return K_DROP;

    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
      return K_SHIFT;

    default:             return K_NONE;
    }
    return K_NONE;
  }


  static void key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    using namespace ui;
    MainUI& self = Globals::ui;
    Key k = keyboardKey(key, mods);
    if (action == GLFW_PRESS) {
      self.keypress(k);
    } else if (action == GLFW_REPEAT) {
      switch (k) {
      case K_RED_UP:
      case K_RED_DOWN:
      case K_BLUE_UP:
      case K_BLUE_DOWN:
      case K_WHITE_UP:
      case K_WHITE_DOWN:
      case K_GREEN_UP:
      case K_GREEN_DOWN:
      case K_LEFT:
      case K_RIGHT:
        self.keypress(k);
      default: break;
      }
    } else if (action == GLFW_RELEASE) {
      self.keyrelease(k);
    }
  }

  void error_callback(int error, const char* description)
  {
    LOGF << description;
  }

  void MainUI::mainRoutine() {
    MainUI& self = Globals::ui;
    GLFWwindow* window;
    NVGcontext* vg = NULL;
    double prevt = 0;

    if (!glfwInit()) {
      LOGE << ("Failed to init GLFW.");
    }

    glfwSetErrorCallback(error_callback);

    LOGD << "So far";
    //glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(drawing::WIDTH, drawing::HEIGHT, "TOP-1", NULL, NULL);
    if (!window) {
      LOGF << "Couldnt create GLFW window";
      glfwTerminate();
      return;
    }

    LOGD << "farther";
    glfwSetWindowAspectRatio(window, 4, 3);
    glfwSetWindowSizeLimits(window, 320, 240, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwSetKeyCallback(window, key);

    glfwMakeContextCurrent(window);

    vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (vg == NULL) {
      printf("Could not init nanovg.\n");
      return;
    }

    glfwSwapInterval(0);

    glfwSetTime(0);
    prevt = glfwGetTime();

    drawing::Canvas canvas(vg, drawing::WIDTH, drawing::HEIGHT);
    drawing::initUtils(canvas);

    LOGI << "Opening GLFW window";
    while (!glfwWindowShouldClose(window) && Globals::running())
      {
        double mx, my, t, dt, spent;
        int winWidth, winHeight;
        int fbWidth, fbHeight;
        float pxRatio;
        float scale;

        t = glfwGetTime();
        dt = t - prevt;
        prevt = t;

        glfwGetCursorPos(window, &mx, &my);
        glfwGetWindowSize(window, &winWidth, &winHeight);
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        // Calculate pixel ration for hi-dpi devices.
        pxRatio = (float)fbWidth / (float)winWidth;
        scale = std::min((float)winWidth/(float)drawing::WIDTH, (float)winHeight/(float)drawing::HEIGHT);
        canvas.setSize(winWidth, winHeight);

        // Update and render
        glViewport(0, 0, fbWidth, fbHeight);

        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        canvas.clearColor(drawing::Colours::Black);
        canvas.begineFrame(winWidth, winHeight);

        canvas.scale(scale, scale);
        self.draw(canvas);

        canvas.endFrame();

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);

        glfwPollEvents();

        spent = glfwGetTime() - t;

        std::this_thread::sleep_for(
                                    std::chrono::milliseconds(int(100/6 - spent*1000)));

      }

    nvgDeleteGL3(vg);

    glfwTerminate();

    Globals::exit();
  }

} // top1::ui
