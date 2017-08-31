// LdbcUpdate4AddForum{forumId=2199023287110, forumTitle='Album 14 of Brian Kelly', creationDate=Thu Sep 13 11:41:44 CEST 2012, moderatorPersonId=6597069776618, tagIds=[12273]}

CREATE (f:Forum {id: "2199023287110", title: 'Album 14 of Brian Kelly', creationDate: 1347529304194});

MATCH (f:Forum {id: "2199023287110"}),
      (p:Person {id:"6597069776618"})
OPTIONAL MATCH (t:Tag)
WHERE t.id IN ["12273"]
WITH f, p, collect(t) as tagSet
CREATE (f)-[:HAS_MODERATOR]->(p)
FOREACH (t IN tagSet| CREATE (f)-[:HAS_TAG]->(t));
