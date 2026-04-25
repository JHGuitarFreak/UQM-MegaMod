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
    private var loggingEnabled = false
    private var logFilePath = ""

    var uqm = "UrQuanMasters"

    override fun loadLibraries() {
        System.loadLibrary("c++_shared")
        System.loadLibrary("SDL2")
        System.loadLibrary(uqm)
    }

    override fun getMainSharedObject(): String {
        return "lib${uqm}.so"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        loggingEnabled = intent.getBooleanExtra("logging_enabled", false)
        logFilePath = intent.getStringExtra("log_file_path") ?: ""
        
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

    override fun getArguments(): Array<String> {
        val arguments = mutableListOf<String>()
        
        if (loggingEnabled && logFilePath.isNotEmpty()) {
            arguments.add("--log=$logFilePath")
        }
        
        return arguments.toTypedArray()
    }

    override fun getLibraries(): Array<String> {
        val libraries = super.getLibraries()
        if (loggingEnabled) {
            
        }
        return libraries
    }
}