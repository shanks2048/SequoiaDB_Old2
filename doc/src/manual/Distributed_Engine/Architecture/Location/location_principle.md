[^_^]:
    位置集原理

位置集是指在一个复制组中，拥有相同位置信息的节点集合，位置集名称即节点的位置信息。

##位置集成员##

位置集成员分为位置集主节点和位置集备节点，一个位置集内有且仅有一个位置集主节点。当位置集主节点与复制组主节点相同时，该位置集又称为复制组的主位置集。

![位置集成员][location_member]

##位置集选举##

位置集选举机制将沿用[复制组选举机制][election]，能够保证当位置集主节点发生故障时，自动在位置集备节点中选举出主节点。

##位置亲和性##

位置亲和性是指两个位置集在地理位置上的亲近关系。当触发数据同步时，系统将优先选择具有亲和性的位置集进行同步。

系统主要依据位置集名称判断亲和性关系，具体规则如下：

- 不区分大小写，名称相同的位置集间具有亲和性关系，例如位置集 GuangDong 和 guangdong。
- 当名称中包含符号点（.）时，以第一个点为分界，前缀相同的位置集间具有亲和性关系，例如位置集 GuangDong.guangzhou 和 GuangDong.shenzhen。

##数据同步##

数据同步主要在以下两种节点间进行：

- 源节点：含有新数据的节点
- 目标节点：请求同步数据的节点

下述将介绍位置集节点间的增量同步和全量同步机制。

###增量同步###

当用户进行增删改操作后，数据库会先将同步日志写入缓冲区，再异步写入节点的同步日志文件中。因此，在增量同步前节点存在以下两种状态：

- 对等状态（Peer）：目标节点请求的同步日志存在于源节点的同步日志缓冲区，源节点可以直接从内存中获取同步日志。
- 远程追赶状态（Remote Catchup）：目标节点请求的同步日志存在于源节点的同步日志文件中。

触发增量同步时，如果目标节点为位置集备节点，系统将在目标节点所在位置集中，优先选择对等状态的节点作为源节点进行同步，其次选择远程追赶状态的节点；如果目标节点为位置集主节点，将优先在与目标节点具有亲和性关系的位置集中，选择对等状态的节点作进行同步。

###全量同步###

触发全量同步时，系统将在目标节点所在位置集内，随机选择一个位置集备节点作为源节点进行同步。如果位置集备节点均不可用，系统将选择位置集主节点作为源节点进行同步；如果位置集主备节点均不可用，将优先在具有亲和性的位置集内，随机选择一个位置集备节点作为源节点进行同步。

##同步一致性策略##

同步一致性策略用于控制复制组内优先同步的节点，实现数据同步的精细化管理。目前支持的策略如下：

- 节点优先策略：随机选取复制组中的节点进行数据同步。
- 位置多数派优先策略：优先保证多数派位置集中均存在数据一致的节点。
- 主位置多数派优先策略：优先保证主位置集中多数派节点数据一致。

同步一致性策略的适用场景可参考[同步一致性][consistency]。

##参考##

关于位置集的相关操作可参考[位置集操作][location_operation]。

[^_^]:
    本文使用的所有引用及链接
[location_member]:images/Distributed_Engine/Architecture/Location/location_member.png
[election]:manual/Distributed_Engine/Architecture/Replication/election.md
[consistency]:manual/Distributed_Engine/Architecture/Location/consistency_strategy.md
[location_operation]:manual/Distributed_Engine/Operation/location_operation.md