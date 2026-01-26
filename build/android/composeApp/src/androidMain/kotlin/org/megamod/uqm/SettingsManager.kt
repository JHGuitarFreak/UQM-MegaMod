package org.megamod.uqm

import android.content.Context
import android.content.SharedPreferences

class SettingsManager(private val context: Context) {
    private val prefs: SharedPreferences = context.getSharedPreferences("uqm_prefs", Context.MODE_PRIVATE)
    
    fun isLoggingEnabled(): Boolean {
        return prefs.getBoolean("logging_enabled", false)
    }
    
    fun setLoggingEnabled(enabled: Boolean) {
        prefs.edit().putBoolean("logging_enabled", enabled).apply()
    }
    
    fun getLogFilePath(): String {
        return "/storage/emulated/0/uqm-megamod/uqm_log.txt"
    }
}