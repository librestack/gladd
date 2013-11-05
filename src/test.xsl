<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
<xsl:output method="text" disable-output-escaping="yes" />

        <xsl:variable name="authuser" select="request/authuser" />
	<xsl:variable name="clientip" select="request/clientip" />

	<xsl:template match="request">
       		<xsl:apply-templates select="data/journal"/>
	</xsl:template>

	<xsl:template match="journal">
		<xsl:text>BEGIN;</xsl:text>
		<xsl:text>INSERT INTO journal (transactdate, description, authuser, clientip) VALUES ('</xsl:text>
		<xsl:value-of select="@transactdate"/>
		<xsl:text>','</xsl:text>
		<xsl:value-of select="@description"/>
		<xsl:text>','</xsl:text>
		<xsl:copy-of select="$authuser"/>
		<xsl:text>','</xsl:text>
		<xsl:copy-of select="$clientip"/>
		<xsl:text>');</xsl:text>
		<fo:inline-sequence>
			<xsl:apply-templates select="debit"/>
			<xsl:apply-templates select="credit"/>
		</fo:inline-sequence>
		<xsl:text>COMMIT;</xsl:text>
	</xsl:template>
	<xsl:template match="debit">
		<xsl:text>INSERT INTO ledger (journal, account, debit) VALUES (currval(pg_get_serial_sequence('journal','id')),'</xsl:text>
		<xsl:value-of select="@account"/>
		<xsl:text>&apos;,&apos;</xsl:text>
		<xsl:value-of select="@amount"/>
		<xsl:text>&apos;);</xsl:text><br/>       
	</xsl:template>
	<xsl:template match="credit">
		<xsl:text>INSERT INTO ledger (journal, account, credit) VALUES (currval(pg_get_serial_sequence('journal','id')),'</xsl:text>
		<xsl:value-of select="@account"/>
		<xsl:text>&apos;,&apos;</xsl:text>
		<xsl:value-of select="@amount"/>
		<xsl:text>&apos;);</xsl:text><br/>       
	</xsl:template>
</xsl:stylesheet>
