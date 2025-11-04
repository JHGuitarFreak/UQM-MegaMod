package org.openmw.utils

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.content.pm.PackageManager
import android.os.Environment
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine

object PermissionHelper {
    suspend fun getManageExternalStoragePermission(activity: Activity): Boolean {
        return suspendCoroutine { continuation ->
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                if (!Environment.isExternalStorageManager()) {
                    val intent = Intent(
                        Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        Uri.parse("package:" + activity.packageName)
                    )
                    activity.startActivityForResult(intent, 24)
                } else {
                    continuation.resume(true) // Permission granted
                }
            } else {
                if (ContextCompat.checkSelfPermission(activity, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED
                ) {
                    ActivityCompat.requestPermissions(
                        activity,
                        arrayOf(Manifest.permission.WRITE_EXTERNAL_STORAGE),
                        23
                    )
                } else {
                    continuation.resume(true) // Permission granted
                }
            }
        }
    }

    fun getTermuxPermission(activity: Activity) {
        if (ContextCompat.checkSelfPermission(activity, "com.termux.permission.RUN_COMMAND") != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(activity, arrayOf("com.termux.permission.RUN_COMMAND"), 26)
        }
    }

    @RequiresApi(Build.VERSION_CODES.R)
    fun handlePermissionResult(
        requestCode: Int,
        grantResults: IntArray,
        continuation: (Boolean) -> Unit
    ) {
        when (requestCode) {
            23 -> {
                if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    continuation(true) // WRITE_EXTERNAL_STORAGE permission granted
                } else {
                    continuation(false) // Permission denied
                }
            }
            24 -> {
                if (Environment.isExternalStorageManager()) {
                    continuation(true) // MANAGE_EXTERNAL_STORAGE permission granted
                } else {
                    continuation(false) // Permission denied
                }
            }
            26 -> {
                if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    println("Termux permission granted.")
                } else {
                    println("Termux permission denied.")
                }
            }
        }
    }
}


