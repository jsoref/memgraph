// LdbcUpdate1AddPerson{personId=35184372093118, personFirstName='Sanjay', personLastName='Anand', gender='male', birthday=Sat Jun 11 02:00:00 CEST 1988, creationDate=Thu Sep 13 12:52:35 CEST 2012, locationIp='103.1.131.183', browserUsed='Firefox', cityId=117, languages=[te, bn, en], emails=[Sanjay35184372093118@gmail.com, Sanjay35184372093118@gmx.com, Sanjay35184372093118@yahoo.com], tagIds=[4, 571, 1187, 2931, 8163, 10222, 12296], studyAt=[Organization{organizationId=3650, year=2007}], workAt=[Organization{organizationId=554, year=2008}]}

// Create the person node.
CREATE (p:Person {id: "35184372093118", firstName: 'Sanjay', lastName: 'Anand', gender: 'male', birthday: 581990400000, creationDate: 1347533555467, locationIP: '103.1.131.183', browserUsed: 'Firefox', speaks: ["te", "bn", "en"], emails: ["Sanjay35184372093118@gmail.com", "Sanjay35184372093118@gmx.com", "Sanjay35184372093118@yahoo.com"]});

// Add isLocatedIn and hasInterest relationships.
MATCH (p:Person {id: "35184372093118"}),
      (c:Place {id: "117"})
OPTIONAL MATCH (t:Tag)
WHERE t.id IN ["4", "571", "1187", "2931", "8163", "10222", "12296"]
WITH p, c, collect(t) AS tagSet
CREATE (p)-[:IS_LOCATED_IN]->(c)
FOREACH(t IN tagSet| CREATE (p)-[:HAS_INTEREST]->(t));

// Add for each studyAt
MATCH (p:Person {id: "35184372093118"}), (u0:Organisation {id: 3650}) CREATE (p)-[:STUDY_AT {classYear: 2007}]->(u0);

// Add for each workAt
MATCH (p:Person {id: "35184372093118"}), (c0:Organisation {id: 554}) CREATE (p)-[:WORK_AT {workFrom: 2008}]->(c0);
