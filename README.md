
# Table of Contents

1.  [main.c:](#org68d06be)
    1.  [int interrupt<sub>flags</sub>](#org53aae37)
    2.  [void main() //main](#orgddb3ace)
    3.  [void input1() //input1](#org61770df)
    4.  [void input2() //input2](#org47ae6d7)
    5.  [void adc<sub>interrupt</sub>() //adc<sub>iterrupt</sub>](#orgd5fb39a)
    6.  [if (interrupt<sub>flags</sub> & 1) //Canal<sub>AR</sub>](#org5c8ed75)
    7.  [if (interrupt<sub>flags</sub> & 2) //Canal<sub>DE</sub>](#orgeada8b2)
    8.  [void handler2() //handler2](#orge852908)
        1.  [truncamento do valor de x, quando próximo do valor de y. Isto serve para o motor não ficar trocando de polaridade repetidamente, tendo em vista que os sinais de entrada são flutuantes, mesmo que não seja intencional;](#org58a12a8)
        2.  [Determina o sentido dos motores (Avanço ou recuo);](#orgb9ee400)
        3.  [Calcula os valores de x e y (vide: Proposta de mapeamento) e aplica como saída PWM dos motores.](#org4a67419)
    9.  [void handler4() //handler4](#org8c87642)
    10. [void handler8() //handler8](#org5ea4340)
    11. [void callibrate() //callibrate](#org6a02518)
2.  [user<sub>defined.h</sub> e user<sub>defined.c</sub>](#orgdbbe40e)
    1.  [void flash<sub>ww</sub>(int \*Data<sub>ptr</sub>, int word) //flash<sub>ww</sub>](#org0a1a2dc)
    2.  [int comparar(int x, int y) //comparar](#org9d8f96c)
3.  [multi.S:](#org3845589)
    1.  [int div(int dividendo, int divisor)](#orga2e016f)
    2.  [int mult(int a, int b)](#org5bc157c)
    3.  [int ab(int val)](#org92c99fd)

Este arquivo tem o objetivo de tentar contextualizar os membros da equipe Maracatronics sobre o projeto da ponte H do robô A La Ursa. Em caso de dúvidas, pode mandar um [email](mailto:ruan_fc@live.com).
O código fonte é dividido em quatro arquivos: [main.c](https://drive.google.com/file/d/1pa1k9cRwgvPIyWGaSXHKw1120n9EiRF3/view?usp=sharing), multi.S e user<sub>defined.h</sub> e user<sub>define.c</sub>. O compilador utilizado foi o msp430-elf-gcc, sucessor do msp430-gcc (este último era utilizado antes). A seguir temos uma lista com uma breve descrição destes arquivos.


<a id="org68d06be"></a>

# main.c:

Este é o código principal. Nele possui o procedimento principal, que é automaticamente executado pelo compilador, e possui os procedimentos que atendem as ISR&rsquo;s. Os procedimentos são:


<a id="org53aae37"></a>

## int interrupt<sub>flags</sub>

Esta variável funciona como um garçom da analogia abaixo. Toda vez que uma interrupção acontece, seus bits mudam de estado para indicar que algum handler ou outro procedimento dentro do if no loop principal deve ser servido. Exemplo,


<a id="orgddb3ace"></a>

## OKAY void main() //[main](main.c)

O main() é dividido em duas principais partes: [Setup](main.c) e [loop](main.c).
A parte do setup realiza todas as configurações iniciais para o funcionamento necessário do robô.
A parte do loop fica sempre percorrendo o estado da variável interrupt<sub>flags</sub>. Funciona como um menu de um restaurante, em que a CPU (o garçom da analogia) busca sempre estar disposto para atender alterações de interrupt<sub>flags</sub> causadas por periféricos do microcontrolador. Ainda na analogia, o garçom trás os pratos para serem servidos, estes pratos seriam os conteúdos dos if&rsquo;s no loop principal. Ao final do atendimento a CPU apaga o bit de interrupt<sub>flags</sub> para indicar que a &rsquo;mesa&rsquo; já foi servida (Cada mesa seria cada tipo de evento que causa uma ISR).


<a id="org61770df"></a>

## KILL void input1() //[input1](main.c)

O canal DE do receptor (eixo y) emite ao microcontrolador sinais em PPM. Estes sinais são capturados através de detecção de borda, usando o modo de captura dos timers (#Vide [slau144](https://www.ti.com/lit/ug/slau144j/slau144j.pdf?ts=1594938619694&ref_url=https%253A%252F%252Fwww.google.com%252F) no capítulo 10). Cada vez que acontece um evento de borda de subida/descida neste canal, esta rotina é chamada para salvar quanto tempo (em ciclos de máquina) se passou desde a última chamada. É através deste método que é possível ler os valores de ppm que são emitidos pelo receptor do robô. Para detalhar mais um pouco o funcionamento desta função, a cada vez que esta função é executada, a borda de captura é trocada. Com isto é possível determinar, dentre as interrupções, quando é capturado valor final ou inicial. Ao término do procedimento, um bit é resetado para indicar que aquele canal (AR) está funcionando corretamente.


<a id="org47ae6d7"></a>

## DONE void input2() //[input2](main.c)

O procedimento input2() tem o início análogo ao input1(), porém para o canal DE (eixo x). A diferença é que esta ISR não serve apenas a IRQ de mudança de borda no canal DE (CCIFG). Além desta funcionalidade, ela serve para atender a IRQ de estouro do timerA0 (TAIFG), que é usada para manipular o comportamento de leds e para fazer polling aos níveis de bateria. Para separar estas duas funcionalidades, é usado um switch, tomando como argumento o registrador do vetor de interrupção TA0IV.


<a id="orgd5fb39a"></a>

## KILL void adc<sub>interrupt</sub>() //[adc<sub>iterrupt</sub>](main.c)

Outra ISR que devemos observar é a rotina adc<sub>interrupt</sub>(). Esta rotina distingue através de um switch qual foi o pino que causou uma interrupção. Após isso, alguma das variáveis envolvidas recebe algum valor que depende da saída do ADC para posterior manipulação. Se for o caso do pino de [FLIP](https://drive.google.com/file/d/12md3JYH-7vf1agyQUhC5Gb0vC6vWDFql/view?usp=sharing) (P1.0), a rotina chama o garçom para poder servir a rotina para calibrar os limites de entrada dos sinais do controle.


<a id="org5c8ed75"></a>

## DONE if (interrupt<sub>flags</sub> & 1) //[Canal<sub>AR</sub>](main.c)

Foi decidido colocar na íntegra dentro do primeiro if dentro do loop principal por ser de tamanho reduzido (3 linhas). A função deste trecho de código serve para mudar a referência do sinal recebido para o canal AR (eixo y). A idéia é que o sinal possua simetria em relação à origem (vide: [Proposta de mapeamento](https://drive.google.com/file/d/1KOykRmHBzwFFcl4DlQlyUH_021YRpbsl/view?usp=sharing)).
Observe que não importa se deltaTy é menor ou maior que zero por uma questão de complemento de 2.


<a id="orgeada8b2"></a>

## DONE if (interrupt<sub>flags</sub> & 2) //[Canal<sub>DE</sub>](main.c)

É análogo ao handler1, porém para o canal DE (eixo x).


<a id="orge852908"></a>

## DONE void handler2() //[handler2](main.c)

O handler2 é o coração do código, é parte principal do MVP. Sua função é fazer:


<a id="org58a12a8"></a>

### truncamento do valor de x, quando próximo do valor de y. Isto serve para o motor não ficar trocando de polaridade repetidamente, tendo em vista que os sinais de entrada são flutuantes, mesmo que não seja intencional;


<a id="orgb9ee400"></a>

### Determina o sentido dos motores (Avanço ou recuo);


<a id="org4a67419"></a>

### Calcula os valores de x e y (vide: [Proposta de mapeamento](https://drive.google.com/file/d/1KOykRmHBzwFFcl4DlQlyUH_021YRpbsl/view)) e aplica como saída PWM dos motores.


<a id="org8c87642"></a>

## DONE void handler4() //[handler4](main.c)

O handler4 serve para gerenciar a indicação da situação atual das baterias através do led vermelho, com o devido cuidado para que o comportamento dos leds não atrapalhem um aos outros. O funcionamento deste procedimento consiste em aproveitar os estouros do TimerA<sub>0</sub> como unidade discreta de tempo. A partir desta unidade de tempo discreta, é manipulado o tempo em que o led vermelho deve ficar aceso ou apagado.


<a id="org5ea4340"></a>

## WAIT void handler8() //[handler8](main.c)

O handler8() serve para reverter o sentido dos motores e trocar os lados dependendo do estado do canal 3 (canal de FLIP).


<a id="org6a02518"></a>

## HOLD void callibrate() //[callibrate](main.c)

O callibrate() serve para guardar na flash os últimos valores de ppm como sendo valores extremos. Após isso, é determinado quem são os valores máximos e mínimos e os valores de offset para leitura de ppm. Esta é a forma usada para realizar calibrações.
Quando o programa é inicializado, o usuário tem a oportunidade realizar uma calibração para os canais AR e DE. A visão geral é que o usuário deve manter o controle ligado com o volante e o gatilho ambos inclinados para direções arbitrárias e ao mesmo tempo o botão de reset na placa deve ser pressionado. Logo em seguida, quando ocorre a primeira leitura do FLIP, se o flipState estiver em estado alto, então uma calibração é realizada.
\#TODO Colocar seleção de valores máximos e mínimos também na condição state = 0.


<a id="orgdbbe40e"></a>

# DONE user<sub>defined.h</sub> e user<sub>defined.c</sub>

O arquivo .h carrega o header da biblioteca feita pelo usuário. O arquivo .c é o source da biblioteca, é neste arquivo que contém o desenvolvimento de cada função declarada em .h. A biblioteca foi criada para fins de organização. Para que o código pareça menor e menos tenebroso e então novos usuários não tenham medo de encará-los. Dentre os suas funções estão:


<a id="org0a1a2dc"></a>

## void flash<sub>ww</sub>(int \*Data<sub>ptr</sub>, int word) //[flash<sub>ww</sub>](user_defined.h)

Serve para escrever na memória flash de forma segura, pela flash (isto é, nenhum trecho do código é passado para RAM atráves da pilha para poder rodar algum código enquanto a gravação é realizada).


<a id="org9d8f96c"></a>

## int comparar(int x, int y) //[comparar](user_defined.h)

Este procedimento, em um contexto em que x e y seriam variáveis unsigned, retorna HIGH = 0xffff se (x > y). Senão, retorna LOW = 0. Pode parecer besteira ou noobice do programador, mas leia o que vem a seguir.
Durante o desenvolvimento do programa, houve um problema de comparação de valores. Optva-se por usar algumas variáveis como unsigned int já que algumas variáveis costumavam ter valores altos e exclusivamente positivos. O problema surgia quando a intenção do programador era comparar dois valores os quais possuiam o bit mais significativo (MSB) diferentes. Acontece que o microcontrolador adota o teste do (MSB) como critério para identificar se o número é negativo, não importando se a variável é unsigned ou não. Dadas estas condições tornou-se necessário criar um método para comparar dois valores
num contexto de valores unsigned.


<a id="org3845589"></a>

# DONE multi.S:

A convenção do msp430-elf-gcc para os registradores é na ordem R12, R13, R14, R15 ao invés do msp430-gcc, que é ao contrário.
Este arquivo contém alguns procedimentos em que o programador optou por usar assembly. Dentre estes procedimentos estão:


<a id="orga2e016f"></a>

## int div(int dividendo, int divisor)

Retorna a divisão inteira do dividendo pelo divisor, tudo multiplicado por 2<sup>16</sup>;


<a id="org5bc157c"></a>

## int mult(int a, int b)

Retorna o produto de a com b dividido por 2<sup>16</sup>;

Estes procedimentos de divisão e multiplicação foram necesários porque o hardware não possui multiplicador, o compilador gera esses códigos cagados e não é uma boa ideia usar pontos flutuantes no msp430, então foi resolvido fazer o nosso próprio.


<a id="org92c99fd"></a>

## int ab(int val)

Retorna o valor absoluto do inteiro de entrada. Houve problemas em usar a biblioteca math.h, então foi decidido fazer o próprio código.

