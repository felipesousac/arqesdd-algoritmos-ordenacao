(() => {
  'use strict';
  const $ = (id) => document.getElementById(id);
  const canvas = $('chart'), ctx = canvas.getContext('2d');
  const names = ['Bubble Sort', 'Selection Sort', 'Insertion Sort', 'Quick Sort', 'Merge Sort'];
  const descriptions = [
    'Percorre a lista repetidamente, trocando vizinhos fora de ordem.',
    'Encontra o menor elemento restante e o coloca na próxima posição.',
    'Insere cada elemento em sua posição dentro da parte já ordenada.',
    'Divide a lista em torno de um pivô e ordena cada lado recursivamente.',
    'Divide a lista ao meio e combina as partes já ordenadas.'
  ];
  let wasm = null, initial = [], frames = [], cursor = 0, timer = null, live = [], highlights = [], sortedFrom = Infinity;
  const counts = { comparisons: 0, swaps: 0 }, colors = { cyan: '#40d7ce', lime: '#c7e66b', orange: '#ff9c61', muted: '#52647b' };

  async function loadEngine() {
    try {
      const factory = (await import('./sort_engine.js')).default;
      wasm = await factory();
      setStatus('Motor C / WebAssembly ativo', true);
      $('sortButton').disabled = false;
    } catch (error) {
      wasm = null;
      setStatus('Motor C / WebAssembly indisponível', false);
      $('sortButton').disabled = true;
      $('sequenceError').textContent = 'Não foi possível carregar o motor C/WebAssembly. Compile o projeto com Emscripten e recarregue a página.';
      $('sequenceError').hidden = false;
      console.error('Falha ao carregar o motor C/WebAssembly:', error);
    }
    applySequence();
  }
  function setStatus(text, ready) { $('engineStatus').textContent = text; $('statusDot').classList.toggle('ready', ready); }
  function readSequence() {
    const raw = $('sequenceInput').value.trim();
    const tokens = raw ? raw.split(/[\s,;]+/).filter(Boolean) : [];
    const values = tokens.map(Number);
    const error = values.length < 2 ? 'Informe pelo menos 2 valores.'
      : values.length > 128 ? 'O limite é de 128 valores para manter a animação fluida.'
      : values.some((value) => !Number.isSafeInteger(value) || value < 1 || value > 1000000) ? 'Use apenas inteiros positivos entre 1 e 1.000.000.'
      : '';
    $('sequenceError').textContent = error;
    $('sequenceError').hidden = !error;
    $('sequenceInput').setAttribute('aria-invalid', String(Boolean(error)));
    return error ? null : values;
  }
  function resetVisualization() {
    stop();
    live = initial.slice(); frames = []; cursor = 0; highlights = []; sortedFrom = Infinity;
    counts.comparisons = counts.swaps = 0; renderMetrics(0,0); draw(); $('stepLabel').textContent = 'Vetor pronto'; $('emptyState').hidden = true; $('timeline').max = 0; $('timeline').value = 0;
  }
  function applySequence() {
    const values = readSequence();
    if (!values) return;
    initial = values;
    resetVisualization();
  }
  function applyOrderedSequence(direction) {
    const count = Number($('sequenceCount').value);
    const error = !Number.isInteger(count) || count < 2 || count > 128
      ? 'Informe uma quantidade inteira entre 2 e 128.'
      : '';
    $('sequenceError').textContent = error;
    $('sequenceError').hidden = !error;
    if (error) return;
    const values = Array.from({ length: count }, (_, index) => direction > 0 ? index + 1 : count - index);
    $('sequenceInput').value = values.join(', ');
    $('sequenceInput').setAttribute('aria-invalid', 'false');
    initial = values;
    resetVisualization();
  }
  const copyFrame = (op, arr, sorted) => ({ op, values: arr.slice(), sorted });
  function runWasm() {
    const mbytes = 24, obytes = 20, ptr = wasm._malloc(initial.length*4);
    if (!ptr) return null;
    wasm.HEAP32.set(initial, ptr>>2);
    const ok = wasm._sort_run(ptr, initial.length, +$('algorithm').value, 2000000); wasm._free(ptr);
    if (!ok) return null;
    const mp = wasm._sort_metrics(), hp = wasm._sort_operations();
    const n = wasm.HEAP32[(mp+4)>>2], comparisons = Number(wasm.HEAP32[(mp+8)>>2]) + Number(wasm.HEAP32[(mp+12)>>2]) * 4294967296;
    const swaps = Number(wasm.HEAP32[(mp+16)>>2]) + Number(wasm.HEAP32[(mp+20)>>2]) * 4294967296;
    const elapsed = wasm.HEAPF64[(mp+24)>>3], total = wasm.HEAP32[(mp+32)>>2];
    const resultPtr = wasm._sort_values(), sorted = Array.from(wasm.HEAP32.subarray(resultPtr>>2,(resultPtr>>2)+n));
    let arr = initial.slice(), out = [copyFrame(null,arr,Infinity)], c=0, s=0;
    for(let k=0;k<total;k++) {
      const p=hp+k*obytes, type=wasm.HEAP32[p>>2], i=wasm.HEAP32[(p+4)>>2], j=wasm.HEAP32[(p+8)>>2];
      if(type===0)c++;
      if(type===1){s++; [arr[i],arr[j]]=[arr[j],arr[i]];}
      if(type===2){arr[i]=wasm.HEAP32[(p+12)>>2];if(wasm.HEAP32[(p+16)>>2])s++;}
      const frame=copyFrame({type,i,j},arr,Infinity);
      frame.comparisons=c;frame.swaps=s;frame.time=elapsed*k/Math.max(1,total);out.push(frame);
    }
    const finalFrame=copyFrame(null,sorted,0); finalFrame.comparisons=comparisons;finalFrame.swaps=swaps;finalFrame.time=elapsed;
    if(out.length===1 || out[out.length-1].values.join(',')!==sorted.join(','))out.push(finalFrame); else out[out.length-1]=finalFrame;
    wasm._sort_free(); return out;
  }
  function startSort() {
    if (!wasm) {
      $('sequenceError').textContent = 'O motor C/WebAssembly não está disponível. Compile o projeto com Emscripten e recarregue a página.';
      $('sequenceError').hidden = false;
      return;
    }
    stop();
    try {
      frames = runWasm();
    } catch (error) {
      frames = null;
      console.error('Falha ao executar o motor C/WebAssembly:', error);
    }
    if (!frames) {
      $('sequenceError').textContent = 'O motor C/WebAssembly não conseguiu executar a ordenação.';
      $('sequenceError').hidden = false;
      return;
    }
    cursor = 0; $('timeline').max = Math.max(0,frames.length-1); $('timelineEnd').textContent = `${frames.length-1} PASSOS`; $('emptyState').hidden = true;
    $('stepLabel').textContent = `${names[+$('algorithm').value]} · ${frames.length-1} passos`; showFrame(0); play();
  }
  function showFrame(index) {
    if(!frames.length)return; cursor=Math.max(0,Math.min(frames.length-1,index)); const f=frames[cursor];live=f.values;highlights=f.op?[f.op.i,f.op.j].filter(i=>i>=0):[];sortedFrom=f.sorted;
    renderMetrics(f.comparisons||0,f.swaps||0,f.time||0); $('timeline').value=cursor;$('progress').innerHTML=`${Math.round(cursor/Math.max(1,frames.length-1)*100)}<span class="percent">%</span>`;$('progressBar').style.width=`${cursor/Math.max(1,frames.length-1)*100}%`;
    $('stepLabel').textContent=f.op?`${f.op.type===0?'Comparação':f.op.type===1?'Troca':'Escrita'} · passo ${cursor} de ${frames.length-1}`:'Vetor final';draw();
  }
  function renderMetrics(c,s,t=0){$('comparisons').textContent=Number(c).toLocaleString('pt-BR');$('swaps').textContent=Number(s).toLocaleString('pt-BR');$('time').textContent=Number(t).toFixed(2);}
  function draw(){const dpr=window.devicePixelRatio||1, rect=canvas.getBoundingClientRect();canvas.width=Math.round(rect.width*dpr);canvas.height=Math.round(rect.height*dpr);ctx.setTransform(dpr,0,0,dpr,0,0);ctx.clearRect(0,0,rect.width,rect.height);if(!live.length)return;
    const gap=live.length>90?1:2, w=Math.max(1,(rect.width-gap*(live.length-1))/live.length), max=Math.max(...live), h=rect.height-8;
    live.forEach((v,i)=>{const bh=Math.max(2,(v/max)*h),x=i*(w+gap),y=rect.height-bh;ctx.fillStyle=highlights.includes(i)?colors.orange:i>=sortedFrom?colors.lime:colors.cyan;const r=Math.min(3,w/2);ctx.beginPath();ctx.roundRect(x,y,w,bh,[r,r,0,0]);ctx.fill();});
  }
  function stop(){if(timer){clearInterval(timer);timer=null;}$('playButton').textContent='▶';$('playButton').setAttribute('aria-label','Reproduzir');}
  function play(){if(!frames.length||cursor>=frames.length-1)return stop();stop();$('playButton').textContent='Ⅱ';$('playButton').setAttribute('aria-label','Pausar');timer=setInterval(()=>{if(cursor>=frames.length-1){stop();return;}showFrame(cursor+1);},Math.max(12,180-(+$('speed').value)*1.55));}
  $('sortButton').addEventListener('click',()=>{const values=readSequence();if(values){initial=values;startSort();}});$('applySequence').addEventListener('click',applySequence);$('ascendingSequence').addEventListener('click',()=>applyOrderedSequence(1));$('descendingSequence').addEventListener('click',()=>applyOrderedSequence(-1));$('shuffleButton').addEventListener('click',applySequence);$('sequenceInput').addEventListener('keydown',e=>{if((e.ctrlKey||e.metaKey)&&e.key==='Enter')applySequence();});$('resetButton').addEventListener('click',()=>{stop();showFrame(0);});$('playButton').addEventListener('click',()=>timer?stop():play());$('stepBack').addEventListener('click',()=>{stop();showFrame(cursor-1);});$('stepForward').addEventListener('click',()=>{stop();showFrame(cursor+1);});$('timeline').addEventListener('input',e=>{stop();showFrame(+e.target.value);});
  $('algorithm').addEventListener('change',e=>{$('algorithmNote').textContent=descriptions[+e.target.value];resetVisualization();});window.addEventListener('resize',draw);
  loadEngine();
})();
