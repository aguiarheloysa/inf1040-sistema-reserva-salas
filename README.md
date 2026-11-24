
# Sistema de Reserva de Salas de Aula

Projeto da disciplina INF1040 — Programação Modular da PUC-Rio. O sistema permite que alunos reservem salas para estudo em grupo e que professores reservem salas para aplicar provas. O núcleo em C11 atende ao menu de terminal e à interface web em React.

## Funcionalidades e regras

- **Cadastro e busca.** A reserva exige um usuário cadastrado com matrícula de 1 a 19 dígitos. A busca considera data, horário, capacidade, quadro, grade de aulas e reservas ativas.
- **Salas e laboratórios.** Cada sala tem código, tipo, capacidade e indicação de quadro. Em `L201`, a letra identifica o edifício e o primeiro algarismo indica o andar. Para reservar um laboratório, um aluno precisa indicar um monitor cadastrado cuja biometria esteja vinculada àquele laboratório.
- **Aulas e autorizações.** Durante uma aula, a reserva só é permitida para prova. O professor solicitante deve ministrar a aula ou ter autorização do ministrante. Se a prova se sobrepuser a mais de uma aula, a regra vale para cada ocorrência.
- **Conflitos e prioridade.** Reservas ativas da mesma sala e data entram em conflito quando os horários se sobrepõem. Horários consecutivos podem coexistir. Uma prova de professor pode substituir reservas conflitantes quando todas são de alunos, faltam pelo menos duas horas para o início de cada uma e o canal aceita os avisos. Se a substituição for recusada, as reservas anteriores permanecem ativas.
- **Cancelamento e histórico.** O titular pode cancelar a própria reserva ativa. O histórico mantém as reservas ativas e canceladas.

## Arquitetura e armazenamento

| Módulo | Responsabilidade |
| --- | --- |
| Usuário | Cadastrar e consultar usuários, seus perfis e endereços de e-mail. |
| Sala | Validar e consultar o inventário, a capacidade, os recursos e os vínculos de biometria. |
| Grade Horária | Controlar as aulas recorrentes, os ministrantes e as autorizações de prova. |
| Reserva | Buscar salas, verificar conflitos e antecedência, coordenar o cancelamento e manter o histórico. |
| Notificação | Enviar os avisos aprovados por Reserva e retornar o resultado da entrega. |
| Tempo | Validar datas e horários e comparar intervalos. |

O terminal guarda os dados em memória durante a sessão. Na interface web, as salas cadastradas são persistidas no PostgreSQL. Usuários, aulas, autorizações, biometrias e reservas ficam em memória enquanto o servidor está rodando.

## Documentação do projeto

- [Sistema de Reserva de Salas de Aula](<Sistema de Reserva de Salas de Aula (1).pdf>) — especificação original.
- [Sistema de Reserva de Salas - Refatorado](<Sistema de Reserva de Salas - Refatorado.md>) — requisitos revisados, arquitetura modular e casos de teste.

## Executar a aplicação

### Terminal

No Windows, com MinGW e GNU Make no PATH, execute na raiz do projeto.

```powershell
mingw32-make CC=gcc
.\build\sistema_reserva.exe
```

No Ubuntu via WSL, execute na raiz do projeto dentro da distribuição.

```sh
make
./build/sistema_reserva
```

Cadastre os usuários e as salas antes de cadastrar aulas, registrar biometrias ou solicitar reservas. Informe as datas como `dd/mm/aaaa`, os horários como `hh:mm` e os dias da semana de 1 (segunda-feira) a 7 (domingo).

### Interface web

O [README do frontend](frontend/README.md) descreve como iniciar o PostgreSQL e o React e como importar as salas de exemplo. O navegador usa a mesma lógica de reservas do terminal por meio de uma API local.

### Notificações por e-mail

O terminal simula os avisos por padrão. Para enviar por SMTP, inicie o executável com a opção `--smtp` e tenha Python 3 no PATH (`python` no Windows ou `python3` no WSL).

| Variável de ambiente | Valor |
| --- | --- |
| `RESERVA_SMTP_HOST` | Servidor SMTP. |
| `RESERVA_SMTP_REMETENTE` | Endereço de remetente autorizado pelo servidor. |
| `RESERVA_SMTP_TLS` | `starttls` (padrão), `ssl` ou `nenhum`. |
| `RESERVA_SMTP_PORTA` | Porta, se diferente do padrão do modo escolhido (587, 465 ou 25). |
| `RESERVA_SMTP_USUARIO` e `RESERVA_SMTP_SENHA` | Credenciais opcionais para os modos com TLS. |

A interface web simula o envio dos avisos. O aceite pelo servidor SMTP indica o recebimento para encaminhamento da mensagem.

## Executar os testes

O Makefile compila os executáveis necessários. Os testes unitários rodam em processos independentes, a integração usa os módulos reais e os testes de interface exercitam o menu de terminal, inclusive os fluxos SMTP com um servidor local de teste.

| Alvo | Suíte |
| --- | --- |
| `test-unit` | Testes unitários dos módulos, incluindo validação de matrícula, conflitos, autorizações e a regra das duas horas. |
| `test-integration` | Interação entre os módulos em C. |
| `test-interface` | Fluxos da interface de terminal. |
| `test` | As três suítes em C. |

### Windows (PowerShell)

1. Tenha MinGW GCC, `mingw32-make` e Python 3 no PATH. Confira com `gcc --version`, `mingw32-make --version` e `python --version`.
2. Abra o PowerShell na raiz do repositório. O comando abaixo usa o caminho deste exemplo.

   ```powershell
   Set-Location C:\dev\inf1040-sistema-reserva-salas
   ```

3. Execute as três suítes juntas.

   ```powershell
   mingw32-make CC=gcc test
   ```

4. Para rodar uma suíte por vez, escolha o comando correspondente.

   ```powershell
   mingw32-make CC=gcc test-unit
   mingw32-make CC=gcc test-integration
   mingw32-make CC=gcc test-interface
   ```

Se aparecer `WinError 4551`, o Controle de Aplicativo do Windows bloqueou a execução de um binário de teste. Use os passos do WSL abaixo.

### Ubuntu via WSL

1. Abra o Ubuntu pelo menu do Windows ou execute `wsl.exe --distribution Ubuntu` no PowerShell.
2. Confira se GCC, GNU Make e Python 3 já estão disponíveis no Ubuntu.

   ```sh
   gcc --version
   make --version
   python3 --version
   ```

   Se os três comandos funcionarem, vá direto para o próximo passo. Se faltar alguma ferramenta, instale os pré-requisitos com uma conta que tenha acesso a `sudo`.

   ```sh
   sudo apt update
   sudo apt install build-essential python3
   ```

3. Entre na raiz do projeto pelo caminho do volume no WSL e rode as três suítes.

   ```sh
   cd /mnt/c/dev/inf1040-sistema-reserva-salas
   make test
   ```

4. Para rodar uma suíte por vez, escolha o comando correspondente.

   ```sh
   make test-unit
   make test-integration
   make test-interface
   ```

Após compilar os testes, é possível rodar um único caso, como `.\build\unit_reserva.exe R12` no PowerShell ou `./build/unit_reserva R12` no WSL. Use `--listar` no executável do módulo para consultar os identificadores disponíveis.

### Teste da API web com PostgreSQL

Tenha Node.js e GCC no PATH do ambiente em que o teste será executado.

1. Configure `frontend/.env` com `DATABASE_URL` apontando para um PostgreSQL acessível no ambiente que vai executar o Node.js. O usuário do banco precisa ter permissão para criar um esquema temporário.
2. Para usar o banco local do projeto, abra o Docker Desktop e rode os comandos abaixo no PowerShell a partir da raiz do repositório.

   ```powershell
   Set-Location frontend
   if (-not (Test-Path .env)) { Copy-Item .env.example .env }
   docker compose up -d --wait
   npm.cmd ci
   npm.cmd test
   ```

3. No Ubuntu via WSL, com o PostgreSQL já acessível e o `.env` configurado, rode os comandos abaixo a partir da raiz do repositório.

   ```sh
   cd frontend
   npm ci
   npm test
   ```

O teste da API cobre cadastro, verificação de matrícula, regras de reserva e persistência das salas após reiniciar o servidor. Consulte os [pré-requisitos do frontend](frontend/README.md).
