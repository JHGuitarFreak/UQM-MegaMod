package org.megamod.uqm

import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.activity.enableEdgeToEdge
import androidx.compose.ui.platform.ComposeView
import org.libsdl.app.SDLActivity

class EngineActivity : SDLActivity() {
    private lateinit var sdlView: View

    override fun loadLibraries() {
        System.loadLibrary("c++_shared")
        System.loadLibrary("openal")
        System.loadLibrary("SDL2")
        System.loadLibrary("uqm")
    }

    override fun getMainSharedObject(): String {
        return "libuqm.so"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        setContentView(R.layout.engine_activity)
        sdlView = getContentView()

        val sdlContainer = findViewById<FrameLayout>(R.id.sdl_container)
        (sdlView.parent as? ViewGroup)?.removeView(sdlView)
        sdlContainer.addView(sdlView)

        val composeViewUI = findViewById<ComposeView>(R.id.compose_overlayUI)
        (composeViewUI.parent as? ViewGroup)?.removeView(composeViewUI)
        sdlContainer.addView(composeViewUI)

        composeViewUI.setContent {
            // Nothing yet
        }
    }
}

