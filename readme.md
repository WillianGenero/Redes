# Roteamento de pacotes por uma rede baseado em sockets UDP
Vitor Antonio Apolinário e Willian Bordignon Genero

### Começando

**1. Clone o repositório em sua pasta de preferência**
```sh
$ git clone https://github.com/vitor-apolinario/computer-network-router.git
```

**2. Abra a pasta**
```sh
$ cd computer-network-router
```

**3. Executando o projeto com os roteadores pré-definidos**
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
- Para cada roteador inicializado será cálculado o vetor distância e enviará para seus vizinhos. Cada roteador irá atualizar seu vetor distância com base nos vetores recebidos até encontrar o melhor caminho para todos os outros roteadores.
- 3 threads são utilizadas:
	-  Uma responsável por ficar recebendo as mensagens (bloqueada enquanto espera).
	-  Uma responsável pelo prompt e posteriormente encaminhar a mensagem.
	-  E a outra enviando periodicamente seu vetor distância aos vizinhos e verificando possíveis quedas de enlaces.
- Para cada mensagem enviada que passa por um roteador intermediário, ele irá ver se é o receptor, se não for irá reencaminhar para o roteador seguinte até o destino, sem visualizar o conteúdo.
- Quando um roteador recebe uma mensagem, será encaminhada uma mensagem de controle contendo a confirmação de recebimento.
- Se após N segundos o transmissor não receber a confirmação um novo pacote será encaminhado.
- Após 3 tentativas sem recebimento de confirmação o transmissor irá desistir.
- A cada 30 segundos cada roteador envia seu vetor distância aos vizinhos.
- Se um roteador ficar 90 segundos sem encaminhar seu vetor distância ele é dado como offline e será removido da topologia.

### Restrições
Para simular a queda de um enlace e necessário finalizar o processo no terminal e após fazer isso ele não poderá voltar ao funcionamento novamente.
