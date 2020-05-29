CREATE DATABASE 'redeveicular';
USE 'redeveicular';
 
CREATE TABLE `bicycle` (
  `id` int(4) NOT NULL,
  `owner` varchar(128) NOT NULL,
  `comsumption` float,
   `list_type` varchar(5) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `id_UNIQUE` (`id`)
)
