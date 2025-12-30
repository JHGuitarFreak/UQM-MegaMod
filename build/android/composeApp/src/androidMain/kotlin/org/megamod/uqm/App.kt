package org.megamod.uqm

import android.content.Context
import android.content.Intent
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.safeContentPadding
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import org.jetbrains.compose.resources.painterResource
import uqm_megamod.composeapp.generated.resources.Res
import uqm_megamod.composeapp.generated.resources.title
import androidx.compose.foundation.layout.padding

@Composable
@Preview
fun App() {
    val context = LocalContext.current
    var isLoggingEnabled by remember { mutableStateOf(false) }

    val darkGreyColorScheme = darkColorScheme(
        primary = Color(0xFF757575),
        onPrimary = Color(0xFF000000),
        primaryContainer = Color(0xFF424242),
        onPrimaryContainer = Color(0xFFE0E0E0),
        
        secondary = Color(0xFFBDBDBD),
        onSecondary = Color(0xFF000000),
        
        surface = Color(0xFF121212),
        onSurface = Color(0xFFE0E0E0),
        
        background = Color(0xFF1A1A1A),
        onBackground = Color(0xFFE0E0E0),
        
        error = Color(0xFFCF6679),

        tertiary = Color(0xFF616161),
        onTertiary = Color(0xFF000000)
    )

    MaterialTheme(
        colorScheme = darkGreyColorScheme
    ) {
        Box(
            modifier = Modifier
                .background(MaterialTheme.colorScheme.background)
                .safeContentPadding()
                .fillMaxSize()
        ) {
            Image(
                painter = painterResource(Res.drawable.title),
                contentDescription = null,
                modifier = Modifier
                    .fillMaxWidth()
                    .align(Alignment.TopCenter)
            )
            Column(
                modifier = Modifier.align(Alignment.Center),
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Button(
                    onClick = { 
                        context.startGame(isLoggingEnabled = isLoggingEnabled)
                    },
                    modifier = Modifier.align(Alignment.CenterHorizontally),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = MaterialTheme.colorScheme.primary,
                        contentColor = MaterialTheme.colorScheme.onPrimary
                    )
                ) {
                    Text("Start Game")
                }
                Spacer(modifier = Modifier.height(24.dp))
                
                Switch(
                    checked = isLoggingEnabled,
                    onCheckedChange = { isLoggingEnabled = it }
                )

                Text(
                    text = "Enable Logging",
                    color = MaterialTheme.colorScheme.onBackground,
                    modifier = Modifier.padding(top = 4.dp)
                )
            }
        }
    }
}

fun Context.startGame(isLoggingEnabled: Boolean = false) {
    val intent = Intent(this, EngineActivity::class.java).apply {
        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        putExtra("logging_enabled", isLoggingEnabled)
        if (isLoggingEnabled) {
            putExtra("log_file_path", "/storage/emulated/0/uqm-megamod/uqm_log.txt")
        }
    }
    this.startActivity(intent)
}