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
      wasm = await factory(); setStatus('Motor WebAssembly ativo', true);
    } catch (_) { setStatus('Modo demonstração JavaScript', true); }
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
      if(type===2)arr[i]=wasm.HEAP32[(p+12)>>2];
      const frame=copyFrame({type,i,j},arr,Infinity);
      frame.comparisons=c;frame.swaps=s;frame.time=elapsed*k/Math.max(1,total);out.push(frame);
    }
    const finalFrame=copyFrame(null,sorted,0); finalFrame.comparisons=comparisons;finalFrame.swaps=swaps;finalFrame.time=elapsed;
    if(out.length===1 || out[out.length-1].values.join(',')!==sorted.join(','))out.push(finalFrame); else out[out.length-1]=finalFrame;
    wasm._sort_free(); return out;
  }
  function runFallback() {
    const arr=initial.slice(), out=[copyFrame(null,arr,Infinity)]; let c=0,s=0;
    const push=(type,i,j)=>{out.push(Object.assign(copyFrame({type,i,j},arr,Infinity),{comparisons:c,swaps:s,time:0}));};
    const cmp=(i,j)=>{c++;push(0,i,j);return arr[i]-arr[j];};
    const swap=(i,j)=>{if(i===j)return;[arr[i],arr[j]]=[arr[j],arr[i]];s++;push(1,i,j);};
    const a=+$('algorithm').value, start=performance.now();
    if(a===0){for(let e=arr.length-1;e>0;e--){let changed=false;for(let i=0;i<e;i++)if(cmp(i+1,i)<0){swap(i,i+1);changed=true;}if(!changed)break;}}
    if(a===1){for(let i=0;i<arr.length-1;i++){let m=i;for(let j=i+1;j<arr.length;j++)if(cmp(j,m)<0)m=j;swap(i,m);}}
    if(a===2){for(let i=1;i<arr.length;i++){let key=arr[i],j=i-1;while(j>=0){c++;push(0,j,i);if(arr[j]<=key)break;arr[j+1]=arr[j];push(2,j+1,-1);j--;}if(j+1!==i){arr[j+1]=key;push(2,j+1,-1);}}}
    if(a===3){const q=(lo,hi)=>{if(lo>=hi)return;let pivot=arr[hi],i=lo;for(let j=lo;j<hi;j++){c++;push(0,j,hi);if(arr[j]<=pivot)swap(i++,j);}swap(i,hi);q(lo,i-1);q(i+1,hi);};q(0,arr.length-1);}
    if(a===4){const m=(lo,hi)=>{if(hi-lo<2)return;let mid=(lo+hi)>>1;m(lo,mid);m(mid,hi);let x=arr.slice(lo,mid),y=arr.slice(mid,hi),i=0,j=0;while(i<x.length&&j<y.length){c++;push(0,lo+i,mid+j);arr[lo+i+j]=x[i]<=y[j]?x[i++]:y[j++];push(2,lo+i+j-1,-1);}while(i<x.length){arr[lo+i+j]=x[i++];push(2,lo+i+j-1,-1);}while(j<y.length){arr[lo+i+j]=y[j++];push(2,lo+i+j-1,-1);}};m(0,arr.length);}
    const elapsed=performance.now()-start;out.forEach(f=>{f.time=elapsed*out.indexOf(f)/Math.max(1,out.length-1);});
    out[out.length-1]=Object.assign(copyFrame(null,arr,0),{comparisons:c,swaps:s,time:elapsed}); return out;
  }
  function startSort() {
    stop(); frames = wasm ? runWasm() : runFallback(); cursor = 0; $('timeline').max = Math.max(0,frames.length-1); $('timelineEnd').textContent = `${frames.length-1} PASSOS`; $('emptyState').hidden = true;
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
  $('sortButton').addEventListener('click',()=>{const values=readSequence();if(values){initial=values;startSort();}});$('applySequence').addEventListener('click',applySequence);$('shuffleButton').addEventListener('click',applySequence);$('sequenceInput').addEventListener('keydown',e=>{if((e.ctrlKey||e.metaKey)&&e.key==='Enter')applySequence();});$('resetButton').addEventListener('click',()=>{stop();showFrame(0);});$('playButton').addEventListener('click',()=>timer?stop():play());$('stepBack').addEventListener('click',()=>{stop();showFrame(cursor-1);});$('stepForward').addEventListener('click',()=>{stop();showFrame(cursor+1);});$('timeline').addEventListener('input',e=>{stop();showFrame(+e.target.value);});
  $('algorithm').addEventListener('change',e=>{$('algorithmNote').textContent=descriptions[+e.target.value];resetVisualization();});window.addEventListener('resize',draw);
  loadEngine();
})();
