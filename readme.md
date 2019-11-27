# Roteamento de pacotes por uma rede baseado em sockets UDP
Vitor Antonio Apolinário e Willian Bordignon Genero

### Começando

**1. Clone o repositório em sua pasta de preferência**
```sh
$ git clone https://github.com/WillianGenero/Redes.git
```

**2. Abra a pasta**
```sh
$ cd Redes
```

**3. Executando o projeto**
 ```sh
$ bin/./init
```

**Ou se preferir compile o arquivo e execute cada roteador individualmente**
 ```sh
$ gcc roteador.c -lpthread -o nomearquivo
```
 ```sh
$ ./nomearquivo
```
Forneça os id's baseando-se nos roteadores configurados nos arquivos enlaces.config e roteador.config.

**4. Envie pacotes**
Em um terminal qualquer digite o ID do roteador destino e após a mensagem de até 100 dígitos que deseja enviar e confirme.

### Funcionamento
- Para cada roteador inicializado será executado o algoritmo de Dijkstra que conhece a topologia completa da rede. Assim saberá o melhor caminho para cada outro roteador.
- 2 threads são utilizadas:
	-  Uma responsável por ficar recebendo as mensagens (bloqueada enquanto espera).
	-  A outra responsável pelo prompt e posteriormente encaminhar a mensagem.
- Para cada mensagem enviada que passa por um roteador intermediário, ele irá ver se é o receptor, se não for irá reencaminhar para o roteador seguinte até o destino, sem visualizar o conteúdo.
- Quando um roteador recebe uma mensagem, será encaminhada uma mensagem de controle contendo a confirmação de recebimento.
- Se após N segundos o transmissor não receber a confirmação um novo pacote será encaminhado.
- Após 3 tentativas sem recebimento de confirmação o transmissor irá desistir.

### Requisitos
Para o funcionamento ideal é necessário que o arquivo de configuração enlace esteja com os IDs linearmente dispostos iniciando por 0.
