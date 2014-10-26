#pragma once
using namespace CPlusPlus;
namespace CodeAtlas
{
	class SymbolLocation
	{
	public:
		//! Default constructor
		SymbolLocation();

		//! Constructor
		explicit SymbolLocation(QString file, int lineNumber = 0, int columnNumber = 0);

		inline const QString &fileName() const { return m_fileName; }
		inline int line() const { return m_line; }
		inline int column() const { return m_column; }
		inline int hash() const { return m_hash; }
		inline bool operator==(const SymbolLocation &other) const
		{
			return line() == other.line() && column() == other.column()
				&& fileName() == other.fileName();
		}

	private:
		QString m_fileName; //!< file name
		int m_line;         //!< line number
		int m_column;       //!< column
		int m_hash;         //!< precalculated hash value for the object, to speed up qHash
	};
	class SymbolInfo
	{
	public:
		enum ElementType {
			Unknown			= 0,

			Class			= (0x1     ),
			Enum			= (0x1 << 1),
			Enumerator		= (0x1 << 2),

			FuncPublic		= (0x1 << 3),
			FuncProtected	= (0x1 << 4),
			FuncPrivate		= (0x1 << 5),

			Namespace		= (0x1 << 6),

			VarPublic		= (0x1 << 7),
			VarProtected	= (0x1 << 8),
			VarPrivate		= (0x1 << 9),

			Signal			= (0x1 << 10),
			SlotPublic		= (0x1 << 11),
			SlotProtected	= (0x1 << 12),
			SlotPrivate		= (0x1 << 13),

			Keyword			= (0x1 << 14),
			Macro			= (0x1 << 15),

			Project			= (0x1 << 16),
			Root			= (0x1 << 17),

			Block			= (0x1 << 18),

			Slot					=   SlotPublic   | SlotProtected | SlotPrivate,
			SignalSlot				=   Signal       | Slot,
			Function				=	FuncPublic   | FuncProtected | FuncPrivate,
			FunctionSignalSlot		=	Function     | SignalSlot,
			FunctionSlot			=   Function     | Signal,
			Variable	            =   VarPublic	 | VarProtected  | VarPrivate,
			NonpublicMember			=   VarProtected | VarPrivate    | FuncProtected | FuncPrivate,
			PublicMember			=   FuncPublic   | VarPublic     | SlotPublic,
			ProtectedMember			=   FuncProtected| VarProtected  | SlotProtected,
			PrivateMember			=   FuncPrivate  | VarPrivate    | SlotPrivate,
			All						=   (0xffffffff),
		};
		enum MatchCase{
			MatchNone   = 0,
			MatchName   = 1,
			MatchType   = 1<<1,
			MatchEither = ((1<<1) | 1)
		};

		SymbolInfo();
		SymbolInfo(const QString &name, const QString &type, ElementType symInfoType = SymbolInfo::Unknown);


		inline const QString &name() const { return m_name; }
		inline const QString &type() const { return m_type; }
		inline ElementType    elementType()     const { return m_elementType; }
		const QString &		  elementTypeName() const;
		bool				  matchNameOrType( const QString& str, MatchCase matchCase = MatchEither)const;

		inline uint           hash()            const { return m_hash; }
		bool                  operator==(const SymbolInfo &other) const;
		bool                  operator< (const SymbolInfo &other) const;
		int					  compare(const SymbolInfo &other) const;

		inline bool		      isEnum()const{return m_elementType == SymbolInfo::Enum;}
		inline bool           isClass()const{return m_elementType == SymbolInfo::Class;}
		inline bool           isFunction()const
		{
			return  m_elementType == SymbolInfo::FuncPrivate    ||	m_elementType == SymbolInfo::FuncProtected  ||
				m_elementType == SymbolInfo::FuncPublic     ||	m_elementType == SymbolInfo::SlotPrivate    ||
				m_elementType == SymbolInfo::SlotProtected  ||	m_elementType == SymbolInfo::SlotPublic     ||
				m_elementType == SymbolInfo::Signal;
		}
		inline bool           isVariable()const
		{
			return  m_elementType == SymbolInfo::VarPrivate     ||   m_elementType == SymbolInfo::VarProtected   ||
				m_elementType == SymbolInfo::VarPublic;
		}
		inline bool           isProject()const{return m_elementType == SymbolInfo::Project;}
		inline bool           isNamespace()const{return m_elementType == SymbolInfo::Namespace;}

		int					  iconTypeSortOrder() const;
		QString               toString(bool showType = false, bool showElementType = false)const;

		static ElementType    elementTypeFromSymbol(const CPlusPlus::Symbol *symbol);
		static const CPlusPlus::Overview& getNamePrinter();

		// return list of smallest type. 
		// For example, input QSharedPointer<ClassA*>, then the output is QSharedPointer and ClassA
		void				  getTypeElements(QStringList& typeList)const;
		void                  getNonBasicTypeElements(QStringList& typeList)const;
		static bool           isBasicTypeWord(const QString& type);

		// primary type
		bool				  isPrimaryType(){return isPrimaryType(m_type);}
		static bool			  isPrimaryType(const QString& type);
	private:
		static int			  elementTypeIdx(ElementType type);
		static void           initBasicTypeWordSet();
		static void			  initPrimaryTypeWordSet();

		ElementType           m_elementType; //!< icon type
		QString               m_name;        //!< symbol name (e.g. SymbolInformation)
		QString               m_type;        //!< symbol type (e.g. (int char))
		uint                  m_hash;        //!< precalculated hash value - to speed up qHash

		static QSet<QString>	    m_basicTypeWords;
		static QSet<QString>		m_primaryTypeWords;			// char, int, unsigned, float, double...
		static const QString		m_elementTypeName[];
		static CPlusPlus::Overview* m_namePrinter;
		static QRegExp				m_typeElementSplitter;
	};
	
	//! qHash overload for QHash/QSet
	inline uint qHash(const SymbolInfo &information)
	{
		return information.hash();
	}


	//! qHash overload for QHash/QSet
	inline uint qHash(const SymbolLocation &location)
	{
		return location.hash();
	}

	// ItemPath class, used to find item in the item tree
	class SymbolPath
	{
	public:
		SymbolPath();
		SymbolPath(QList<SymbolInfo>& symbolPath);
		~SymbolPath(){}

		void                     clear();

		const QList<SymbolInfo>& getSymbolPath()const;
		void                     setSymbolPath(QList<SymbolInfo>& symbolPath);

		const SymbolInfo* getFirstSymbol()const{return getSymbol(0);}
		const SymbolInfo* getLastSymbol()const{return getSymbol(m_symbolPath.size()-1);}
		const SymbolInfo* getSymbol( int ithSymbol )const;
		const SymbolInfo* getLastProjSymbol()const;
		int                      getSymbolCount()const;
		void                     getProjectPath(SymbolPath& path)const;
		SymbolPath               getProjectPath()const;
		// a top level item is:
		// 1.class
		// 2.function which has no parent class
		void                     getTopLevelItemPath(SymbolPath& path)const;

		SymbolPath&	             addChildSymbol( const SymbolInfo& symbol );
		SymbolPath&	             addChildPath(const SymbolPath& parentPath);
		SymbolPath&	             addParentSymbol(const SymbolInfo& symbol);
		SymbolPath&	             addParentPath(const SymbolPath& parentPath);

		bool					 operator==(const SymbolPath& p)const;
		unsigned                 findCommonLength(const SymbolPath& p)const;

		uint                     hash()const{return m_hash;}
		QString                  toString(bool showType = false, bool showElementType = false)const;

		// find a symbol's complete definition and its corresponding symbol path
		static void              findFunctionDefPath( Function* funSymbol, const LookupContext& context, SymbolPath& itemPath );
	private:
		void                     rehash();
		QList<SymbolInfo> m_symbolPath;
		uint                     m_hash;
	};

	inline uint qHash(const SymbolPath &item)
	{
		return item.hash();
	}
}