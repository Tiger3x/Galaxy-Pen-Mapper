# Diagnóstico de duas fronteiras — preparação, não instalação

**Atualização posterior nesta data:** há agora um candidato assinado e gerenciador de instalação/restauração, ainda não executados. A estratégia usa serviços separados, lista legacy por instância e verificação da pilha real. Veja [implantação e recuperação](P007-DIAGNOSTIC-DEPLOYMENT.md). As seções abaixo registram a entrega inicial sem pacote instalável.

## Entrega de 23/09/2026

Protótipo compilável, separado do Corrector 0.3.1. Não altera o app, a curva ou o driver carregado. Os binários de laboratório ficam em `build-diagnostic`, não nos pacotes de produção. Não há INF, catálogo, assinatura ou instalador do experimento; não deve ser carregado manualmente. Não usar kdmapper, carregamento forçado ou remoção experimental do Samsung.

Componentes:

- `driver/DiagnosticProbe.c` / `GalaxyPenDiagnostic.vcxproj`: mesmo código compilado como observador A e B, com canais exclusivos de administrador distintos.
- `diagnostics/collector.cpp`: consulta o estado real de COL01; captura limitada a 30 segundos somente com a pilha exata; salva as duas sequências e um relatório.
- `diagnostics/Pairing.h`: pareamento por identidade opaca de requisição, sessão e intervalo temporal. Não usa coordenadas ou proximidade temporal isoladamente para alegar a mesma amostra.
- `diagnostics/tests.cpp`: testes de protocolo, hash, fila limitada, perdas, identidade reutilizada, ambiguidades, buffers inválidos e bloqueio de pilha incorreta.
- `tools/build-pressure-diagnostic.ps1`: compila com Ninja e WDK/análise estática, executa os testes e gera manifesto que declara `Installable=false`, `Installed=false`, `Signed=false`, `HardwareValidated=false`.

## Posição exigida, do topo para baixo

```text
GalaxyPenDiagA        captura a resposta depois do Samsung
PenS2Helper           componente investigado, preservado
GalaxyPenDiagB        captura a resposta antes do Samsung
mshidkmdf            caminho HID existente
```

O coletor consulta `DEVPKEY_Device_Stack` diretamente, verifica dispositivo iniciado/sem problema e compara a lista completa. Não aceita o Corrector no experimento: suas alterações tornariam a comparação ambígua. Não aceita inversão A/B, componentes extras ou observadores ausentes. Reconfere a pilha durante e ao final da captura. `--preflight` é somente leitura e não abre canais de captura.

**Pendência de implantação:** o INF base Samsung não define níveis para garantir duas posições diferentes. `FilterPosition=Upper` não significa "imediatamente antes/depois do Samsung"; a ordem entre filtros sem níveis não é garantida. `Lower` se refere ao driver de função, não automaticamente ao filtro Samsung. Portanto não foi criado um INF que prometa uma posição não demonstrada. A estratégia de posicionamento e o rollback precisam ser revisados antes de tornar este protótipo instalável.

Referência: [Microsoft — ordenação dos filtros](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/device-filter-driver-ordering). Os níveis são definidos pelo pacote base; uma extensão não deve inventar níveis que o pacote base não declarou.

## Observação sem transformação

O filtro encaminha `IRP_MJ_READ`, copia até o relatório esperado de 15 bytes para uma fila de diagnóstico e devolve os bytes, estado e contagem originais. Não injeta relatórios, não altera flags/pressão, não emite comandos proprietários nem conhece o protocolo de ativação do Corrector. Demais tipos de requisição não são interceptados pelo observador.

Fila fixa de 2.048 registros por observador; quando cheia, perde observações e conta perdas, sem bloquear entrada esperando o coletor. Não aloca memória no caminho de conclusão. Canal exclusivo restrito a administrador/SYSTEM. Registro começa apenas por comando explícito, expira após 30 segundos sem leitura do coletor e desarma em fechamento, transição de energia ou alteração de anexação. Reativação exige novo início explícito. Chaves de pareamento são apagadas ao desarmar; o fechamento também limpa a fila.

Essas propriedades estão implementadas, mas não constituem comprovação de estabilidade em kernel real. Não executar o protótipo na máquina de uso antes da revisão de implantação e de um caminho de recuperação.

## Como comparar sem inventar correspondências

Os observadores calculam SipHash-2-4 da identidade do IRP usando a mesma chave aleatória por sessão. O CSV contém apenas o identificador opaco, nunca endereço de kernel ou chave. O coletor usa o relógio monotônico comum do kernel e exige que o intervalo observado em B esteja contido no intervalo em A. Identidades reutilizadas em leituras posteriores são separadas pelos intervalos. Duplicidades, mudança de sessão, estados inválidos e ambiguidades são rejeitados.

Se o Samsung recriar requisições em vez de encaminhar o mesmo IRP, não haverá pareamento confiável. Esse resultado é **inconclusivo**, não prova de pressão modificada. Pareamento usa hash de 64 bits com chave; colisão não é matematicamente impossível. Um resultado significativo deve se repetir em muitas amostras válidas, não depender de um único par.

Saídas previstas: `before-samsung.csv`, `after-samsung.csv`, `result.txt`, em diretório novo. O relatório separa perdas, inválidos, não pareados, igualdade, alteração de pressão e alteração de outros campos. Mostra também faixa e saturação brutas por fronteira, sem confundir agregados com pares individuais. Limite adicional de 200.000 registros por lado no coletor. Dados crus são preservados mesmo quando a captura é interrompida após o início, quando for possível gravá-los.

## Verificações executadas nesta preparação

- Compilação Ninja do coletor e testes com avisos tratados como erro.
- Testes sintéticos, incluindo vetor de referência SipHash para oito bytes, overflow/FIFO, 10.000 reutilizações de identidade e rejeição de pilhas erradas.
- Compilação dos observadores A/B com WDK e análise estática sem defeitos reportados.
- Preflight real: retornou bloqueio esperado na pilha atual `Corrector > Samsung > mshidkmdf`. Não abriu canais, não capturou caneta e não alterou o dispositivo.
- Nenhuma instalação, reinicialização, alteração de assinatura/segurança ou substituição do app atual.

## Antes de autorizar uma futura instalação

1. Resolver o posicionamento por procedimento suportado e revisado, sem supor ordem a partir dos nomes. Não modificar o pacote base Samsung silenciosamente.
2. Definir instalador específico, alvo COL01 exato, registro do estado anterior e backup recuperável do Corrector. Nunca aplicar filtro a toda a classe HID ou ao pai I2C por conveniência.
3. Definir e revisar remoção apenas dos dois observadores e restauração da pilha anterior. Se o Windows exigir reboot, parar, não forçar descarga.
4. Validar INF/assinaturas/catálogo e controles de segurança desse futuro pacote; solicitar autorização separada para instalação.
5. Só depois verificar pilha, leituras, perdas e pareamento com movimento/pressão confortável. Se ambas as fronteiras mostrarem 4095, essa medição não dará acesso à força física ausente.

**Estado de entrega:** preparado e testado offline; instalação e comparação física ainda não prontas nem realizadas.
