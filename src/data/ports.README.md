# Ports dataset

This dataset is retrieved from: https://datacatalog.worldbank.org/search/dataset/0038118


## CONTENTS AND LAYOUT OF UN/LOCODE: CODES AND ABBREVIATIONS USED

The following is the content of: https://service.unece.org/trade/locode/Service/LocodeColumn.htm#Function

1. The UN approved Location codes are presented in 11 columns, with the following content (column designations between quotes):


Note: This description applies to UN/LOCODE Web pages presentation only. For UN/LOCODE downloadable files in MS Access, CSV or Text file formats, the column "LOCODE" is splitted in 2 columns for an easier use: Country and Code.

1.1 Column "Ch"  (Change Indicator)

Every changed UN/LOCODE from previous issue has a change indicator in this field. The following indicators are used:

 +	added entry
 #	Change in the location name
 X	entry to be removed in the next issue
 |	entry has been changed
 =	reference entry
 !	US locations with duplicated IATA code, under review
1.2 Column "LOCODE"

The two first digits in "LOCODE" column indicates the country in which the place is located.   The values used concur with the  ISO 3166 alpha-2 Country Code. You can also review a complete list of countries and their code value. In cases where no ISO 3166 country code element is available, e.g. installations in international waters or international cooperation zones, the code element "XZ", available for user assignment in accordance with clause 8.1.3 of ISO 3166-1/1997, will be used.

In the next part of the "LOCODE" column you can find a 3-character code for the location.   The 3-character code element for the location will normally comprise three letters. However, where all permutations available for a country have been exhausted, the numerals 2-9 may also be used.

Thus, a complete UN/LOCODE is a combination of a 2-character country code and a 3-character location code, e.g. BEANR is known as the city of Antwerp (ANR) which is located in Belgium (BE). For ease of reading, the country and location code elements are usually separated by a space. In actual use, this space could be suppressed.

1.3 Column "Name"

Column "Name" shows the names of the locations which have been allocated a UN/ LOCODE. Place names are given, whenever possible, in their national language versions as expressed in the Roman alphabet using the 26 characters of the character set adopted for international trade data interchange, with diacritic signs, when practicable.  Diacritic signs may be ignored, and should not be converted into additional characters (e.g., Göteborg may be read as Goteborg, rather than Goeteborg, Gothenburg, Gotembourg, etc.), in order to facilitate reproduction in the national language. In countries with more than one national language, place names may be different in the respective languages.  In such cases, more than one name version maybe included, followed by other versions placed within brackets, e.g.:

Abo (Turku)
Turku (Abo)
As a service to users, names that have been changed may be included for reference.  Such alternative name versions are included as a transitional measure after a name change; they are followed by an equal sign (=), e.g.:

Peking = Beijing
Leningrad = Saint Petersburg

The code element will be shown only under the new name.

In some cases national location names are represented differently in different languages.  This may lead to misunderstandings which can cause disputes in interpretation of transport and other contracts, in documentary credits, etc.  For any such differing but widely used  name forms that are known to or reported to the UN Secretariat, reference to the preferred name version may be made, followed by an equal (=) sign, e.g.:

Flushing = Vlissingen
Munich = München
A place name may be followed, after a comma sign, by an indication of geographical or administrative significance, such as the name of an island on which the place is located, e.g. Bandung, Java; Taramajima, Okinawa.

There may be subordinate entities under a place name, e.g. different airports serving the same main location, outlying ports, freight terminals, etc.  If a separate code element has been assigned to such a location, the name of the sublocation is added after the main name, separated by a hyphen (-), e.g.

GB LHR London-Heathrow Apt

CL ULC Santiago-Los Cerrillos Apt

The sublocation name is also listed in its proper alphabetic name order place, followed by an oblique stroke (slash) (/) and the name of the main place to which it belongs, e.g.:

GB LHR Heathrow Apt/London

CL ULC Los Cerrillos Apt/Santiago

Abbreviations used in Column 4 include:

 Apt	for Airport
 I.	for Island(s)
 Pto	for Puerto
 Pt	for Port
 St	for Saint
1.4 Column "NameWoDiacritics"

Column "NameWoDiacritics" shows the names of the locations which have been allocated a UN/LOCODE without diacritic signs.

1.5 Column "SubDiv"  (Subdivision)

Column "Subdivision" is intended to contain the ISO 1-3 character alphabetic and/or numeric code for the administrative division of the country concerned (state, province, department, etc.), as included in International Standard ISO 3166-2/1998 and when deemed desirable to enhance the use of the code, or when requested by the country concerned.

The country code element is not shown in this column.  Only the latter part of the complete ISO 3166-2 code element (after the hyphen) is shown, as a qualifier to the location name.

1.6 Column "Function"

This column contains a 8-digit function classifier code for the location, where:

 0	A value "0" in the first position specifies that the functional use of a location is not known and is to be specified.
 1	Specifies that the location is a Port, as defined in UN/ECE Recommendation 16.
 2	Specifies that the location is a Rail terminal.
 3	Specifies that the location is a Road terminal.
 4	Specifies that the location is an Airport.
 5	Specifies that the location is a Postal exchange office.
 6	Value reserved for multimodal functions, ICDs etc.
 7	Value reserved for fixed transport functions (e.g. oil platform).
 B	Specifies that the location is Border crossing.
1.7 Column "Status"

This column is intended to indicate the status of the entry by a 2-character code, e.g. whether approved by Government, by Customs, or based on a user requirement not necessarily recognised by an authority, etc. It is also intended to show the status of checking, e.g. that function indicators are not verified.

The following codes are used at present:

 AA	Approved by competent national government agency
 AC	Approved by Customs Authority
 AF	Approved by national facilitation body
 AI	Code adopted by international organisation (IATA or ECLAC)
 AS	Approved by national standardisation body
 RL	Recognised location - Existence and representation of location name confirmed by check against nominated gazetteer or other reference work
 RN	Request from credible national sources for locations in their own country
 RQ	Request under consideration
 RR	Request rejected
 QQ	Original entry not verified since date indicated
 XX	Entry that will be removed from the next issue of UN/LOCODE
1.8 Column "Date"

The column "Date" displays the last date when the location was updated/entered.

1.9 Column "IATA"

The IATA code for the location if different from location code in column LOCODE (second part of the code).

1.10 Column "Coordinates"

This column contains the geographical coordinates (latitude/longitude) of the location, if there is any.

In order to avoid unnecessary use of non-standard characters and space, the following standard presentation is used:

0000lat 00000long

(lat - Latitude: N or S ; long – Longitude: W or E, only one digit, capital letter)

Where the last two rightmost digits refer to minutes and the first two or three digits refer to the degrees for latitude and longitude respectively. In addition, you must specify N or S for latitude and W or E for longitude, as appropriate.
1.11 Column "Remarks"

This column may contain some general remarks regarding the UN/LOCODE in question. Usually it gives more information regarding the creation or change of a UN/LOCODE.