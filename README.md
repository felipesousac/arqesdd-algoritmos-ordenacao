# Ordena — visualizador de algoritmos

Aplicação web em português para visualizar Bubble Sort, Selection Sort, Insertion Sort, Quick Sort e Merge Sort. O motor de ordenação é escrito em C e compilado para WebAssembly com Emscripten; a interface usa JavaScript, HTML, CSS e Canvas.

O projeto usa GNU Make e um servidor HTTP local. No Windows, o caminho deste guia usa WSL 2 com Ubuntu para que os mesmos comandos do projeto funcionem sem adaptações.

## Windows 10/11 com WSL 2

### 1. Instale o WSL e o Ubuntu

Abra o PowerShell como administrador e execute:

```powershell
wsl --install -d Ubuntu
```

Reinicie o computador se o Windows solicitar. Abra o Ubuntu pelo menu Iniciar e crie seu usuário e senha Linux. Consulte o [guia oficial do WSL](https://learn.microsoft.com/windows/wsl/install) caso o comando não esteja disponível ou o WSL já esteja instalado.

### 2. Instale os pacotes necessários no Ubuntu

No terminal do Ubuntu (não no PowerShell):

```bash
sudo apt update
sudo apt install -y git python3 make
```

### 3. Instale e ative o Emscripten

Ainda no Ubuntu, baixe e instale o SDK oficial:

```bash
cd ~
git clone https://github.com/emscripten-core/emsdk.git
cd ~/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
emcc --version
```

O último comando deve imprimir a versão do compilador. A ativação via `source` vale para o terminal atual. Sempre que abrir um terminal Ubuntu novo, execute:

```bash
source ~/emsdk/emsdk_env.sh
```

### 4. Abra a pasta do projeto no WSL

Se o projeto está no disco C:, acesse-o pelo caminho `/mnt/c`. Por exemplo:

```bash
cd /mnt/c/Users/SEU_USUARIO/Documents/Caminho/Para/o/projeto
```

Substitua `SEU_USUARIO` e o restante pelo caminho real. Para melhor desempenho, a Microsoft recomenda manter os arquivos de projetos usados no Linux dentro do sistema de arquivos Linux, por exemplo em `~/projetos/ordena`.

### 5. Compile e execute

Na pasta do projeto:

```bash
make
make serve
```

Abra [http://localhost:8000](http://localhost:8000) no navegador do Windows. O servidor permanece em execução enquanto o terminal estiver aberto; pressione `Ctrl+C` para encerrá-lo.

## Linux

### 1. Instale os pacotes necessários

Em distribuições baseadas em Debian ou Ubuntu:

```bash
sudo apt update
sudo apt install -y git python3 make
```

Em outra distribuição, instale os pacotes equivalentes: Git, Python 3 e GNU Make.

### 2. Instale e ative o Emscripten

O Emscripten SDK requer Python 3.8 ou mais recente e Git. Os comandos abaixo instalam a versão mais recente do SDK oficial:

```bash
cd ~
git clone https://github.com/emscripten-core/emsdk.git
cd ~/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
emcc --version
```

O último comando deve imprimir a versão do compilador. Para disponibilizar `emcc` em um novo terminal, execute:

```bash
source ~/emsdk/emsdk_env.sh
```

### 3. Compile e execute o projeto

Vá para a pasta do projeto e rode:

```bash
cd /caminho/para/o/projeto
make
make serve
```

Abra [http://localhost:8000](http://localhost:8000) no navegador. Pressione `Ctrl+C` no terminal para encerrar o servidor.

## Uso do visualizador

1. Digite de 2 a 128 inteiros positivos entre 1 e 1.000.000 no campo **Sua sequência**. Separe os valores por vírgula, espaço ou ponto e vírgula.
2. Escolha o algoritmo.
3. Clique em **Aplicar sequência** para atualizar a visualização, ou em **Ordenar** para validar e começar diretamente.
4. Use os controles do player para pausar, avançar, voltar, alterar a velocidade ou reiniciar a reprodução.

O campo começa com uma sequência de exemplo que pode ser substituída. Reaplicar a sequência ou trocar o algoritmo mantém os valores informados.

## Comandos úteis

- `make` compila `src/sort_engine.c` para `web/sort_engine.js` e `web/sort_engine.wasm` quando `emcc` está ativo no terminal.
- `make serve` inicia o servidor de desenvolvimento em `http://localhost:8000`.
- `make clean` remove os arquivos gerados da compilação.

Se `emcc` não estiver disponível, `make` avisa e não compila o módulo. A página então usa um modo de demonstração com versões JavaScript dos algoritmos. Nesse modo, a métrica de tempo se refere ao JavaScript. Se `emcc --version` funcionar num terminal e não em outro, ative o ambiente do SDK no terminal usado para rodar `make`.

## Como o histórico é medido

- O motor C aceita de 1 a 4096 inteiros e pode registrar até 2 milhões de operações por execução. A interface limita a entrada a 128 valores para manter a animação fluida.
- As operações são comparação (`COMPARE=0`), troca (`SWAP=1`) e escrita (`WRITE=2`). Insertion e Merge registram deslocamentos e cópias como escritas, não como trocas.
- Comparações contam avaliações entre valores; trocas contam mudanças efetivas de posição.
- O tempo do Wasm mede a execução do algoritmo C com `emscripten_get_now()`, sem incluir a renderização nem a leitura do histórico.
- Se o limite do histórico for atingido, a ordenação e as métricas continuam corretas, mas operações excedentes não aparecem na animação.

## Referências de instalação

- [Download e instalação oficial do Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
- [Comandos e ativação do emsdk](https://emscripten.org/docs/tools_reference/emsdk.html)
- [Instalar WSL](https://learn.microsoft.com/windows/wsl/install)
