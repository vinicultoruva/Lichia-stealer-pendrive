package br.com.lichia.app

import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.util.UUID

object TgClient {
    const val TG_LIMIT = 49L * 1024 * 1024

    fun sendText(token: String, chatId: String, text: String): Boolean = try {
        val body = "chat_id=" + URLEncoder.encode(chatId, "UTF-8") +
            "&text=" + URLEncoder.encode(text.take(3500), "UTF-8")
        val conn = URL("https://api.telegram.org/bot$token/sendMessage")
            .openConnection() as HttpURLConnection
        conn.requestMethod = "POST"
        conn.doOutput = true
        conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded")
        conn.connectTimeout = 15000
        conn.readTimeout = 30000
        conn.outputStream.use { it.write(body.toByteArray(Charsets.UTF_8)) }
        val ok = conn.responseCode in 200..299
        conn.disconnect()
        ok
    } catch (e: Exception) {
        false
    }


    fun sendDocument(token: String, chatId: String, file: File, caption: String): Boolean = try {
        val boundary = "----Lichia" +
            UUID.randomUUID().toString().replace("-", "").take(16)

        val conn = URL("https://api.telegram.org/bot$token/sendDocument")
            .openConnection() as HttpURLConnection
        conn.requestMethod = "POST"
        conn.doOutput = true
        conn.setRequestProperty("Content-Type", "multipart/form-data; boundary=$boundary")
        conn.connectTimeout = 15000
        conn.readTimeout = 180000

        conn.outputStream.use { out ->
            out.write(("--$boundary\r\n" +
                "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n")
                .toByteArray(Charsets.UTF_8))
            out.write(chatId.toByteArray(Charsets.UTF_8))
            out.write("\r\n".toByteArray(Charsets.UTF_8))

            if (caption.isNotEmpty()) {
                out.write(("--$boundary\r\n" +
                    "Content-Disposition: form-data; name=\"caption\"\r\n\r\n")
                    .toByteArray(Charsets.UTF_8))
                out.write(caption.toByteArray(Charsets.UTF_8))
                out.write("\r\n".toByteArray(Charsets.UTF_8))
            }

            out.write(("--$boundary\r\n" +
                "Content-Disposition: form-data; name=\"document\"; " +
                "filename=\"${file.name}\"\r\n" +
                "Content-Type: application/octet-stream\r\n\r\n")
                .toByteArray(Charsets.UTF_8))
            file.inputStream().use { it.copyTo(out, 64 * 1024) }
            out.write("\r\n--$boundary--\r\n".toByteArray(Charsets.UTF_8))
        }


        val ok = conn.responseCode in 200..299
        conn.disconnect()
        ok
    } catch (e: Exception) {
        false
    }


    fun sendChunked(token: String, chatId: String, zip: File, workDir: File) {
        val size = zip.length()
        val parts = ((size + TG_LIMIT - 1) / TG_LIMIT).toInt()
        zip.inputStream().use { ins ->
            for (i in 1..parts) {
                val expect = minOf(TG_LIMIT, size - (i - 1) * TG_LIMIT).toInt()
                val chunk = ByteArray(expect)
                var off = 0
                while (off < expect) {
                    val n = ins.read(chunk, off, expect - off)
                    if (n < 0) break
                    off += n
                }
                if (off == 0) break
                val tmp = File(workDir, "lichen.part$i.zip")
                tmp.writeBytes(chunk.copyOf(off))
                try {
                    sendDocument(token, chatId, tmp,
                        "parte $i/$parts — ${zip.name}")
                } finally {
                    tmp.delete()
                }
            }
        }
    }
}
