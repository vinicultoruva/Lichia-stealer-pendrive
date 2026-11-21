package br.com.lichia.app

import android.content.Context

object Prefs {
    const val MARKER = "lichen.config"
    private const val FILE = "lichen"

    data class Cfg(val token: String, val chatId: String,
                   val mode: String, val maxMb: Int)

    fun save(ctx: Context, c: Cfg) {
        ctx.getSharedPreferences(FILE, Context.MODE_PRIVATE).edit()
            .putString("token", c.token)
            .putString("chat_id", c.chatId)
            .putString("modo", c.mode)
            .putInt("max_mb", c.maxMb)
            .apply()
    }

    fun read(ctx: Context): Cfg {
        val p = ctx.getSharedPreferences(FILE, Context.MODE_PRIVATE)
        return Cfg(
            p.getString("token", "") ?: "",
            p.getString("chat_id", "") ?: "",
            p.getString("modo", "all") ?: "all",
            p.getInt("max_mb", 200))
    }
}
